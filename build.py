#!/usr/bin/env python3
"""
Unified Build Script for op project.

Usage:
    python build.py [options]

Examples:
    python build.py                                # Release + VS2022 + x64
    python build.py -t Debug                       # Debug + VS2022 + x64
    python build.py -t Release -g nmake            # Release + NMake + x64
    python build.py -t RelWithDebInfo -g nmake     # RelWithDebInfo + NMake + x64
    python build.py -t Release -g vs2019           # Release + VS2019 + x64
    python build.py -t Release -a x86              # Release + VS2022 + x86
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path

# ── Generator definitions ──────────────────────────────────────────

GENERATORS = {
    "nmake":  {"cmake": "NMake Makefiles", "ide": False},
    "ninja":  {"cmake": "Ninja",           "ide": False},
    "vs2019": {"cmake": "Visual Studio 16 2019", "ide": True},
    "vs2022": {"cmake": "Visual Studio 17 2022", "ide": True},
}

BUILD_TYPES = ["Debug", "Release", "RelWithDebInfo"]
ARCHITECTURES = ["x86", "x64"]

# ── Helper functions ───────────────────────────────────────────────

def find_vcvarsall() -> str | None:
    """Search common paths for vcvarsall.bat."""
    candidates = [
        r"C:\Program Files\Microsoft Visual Studio\2022\{}\VC\Auxiliary\Build\vcvarsall.bat",
        r"C:\Program Files (x86)\Microsoft Visual Studio\2019\{}\VC\Auxiliary\Build\vcvarsall.bat",
    ]
    for pattern in candidates:
        for edition in ("Community", "Professional", "Enterprise"):
            path = pattern.format(edition)
            if os.path.isfile(path):
                return path
    return None


def setup_msvc_env(arch: str) -> dict[str, str]:
    """Run vcvarsall.bat and capture the resulting environment variables."""
    vcvarsall = find_vcvarsall()
    if not vcvarsall:
        print("[ERROR] Could not find vcvarsall.bat. Please install Visual Studio.")
        sys.exit(1)

    vcvars_arch = "amd64" if arch == "x64" else "x86"
    print(f"[INFO] Setting up MSVC environment via: {vcvarsall} {vcvars_arch}")

    # Run vcvarsall and dump the environment
    cmd = f'"{vcvarsall}" {vcvars_arch} && set'
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"[ERROR] vcvarsall.bat failed:\n{result.stderr}")
        sys.exit(1)

    # Parse environment variables
    env = {}
    for line in result.stdout.splitlines():
        if "=" in line:
            key, _, value = line.partition("=")
            env[key] = value
    return env


def run(cmd: list[str], env: dict[str, str] | None = None) -> None:
    """Run a command, printing it first, and exit on failure."""
    print(f"[RUN] {' '.join(cmd)}\n")
    result = subprocess.run(cmd, env=env)
    if result.returncode != 0:
        print(f"\n[ERROR] Command failed with exit code {result.returncode}")
        sys.exit(result.returncode)


# ── Main ───────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Unified build script for the op project.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
examples:
  python build.py                                # Release + VS2022 + x64
  python build.py -t Debug                       # Debug + VS2022 + x64
  python build.py -t Release -g nmake            # Release + NMake + x64
  python build.py -t RelWithDebInfo -g vs2019    # RelWithDebInfo + VS2019 + x64
  python build.py -t Release -a x86              # Release + VS2022 + x86
""",
    )
    parser.add_argument(
        "-t", "--type",
        choices=BUILD_TYPES,
        default="Release",
        help="Build type (default: Release)",
    )
    parser.add_argument(
        "-g", "--generator",
        choices=GENERATORS.keys(),
        default="vs2022",
        help="CMake generator (default: vs2022)",
    )
    parser.add_argument(
        "-a", "--arch",
        choices=ARCHITECTURES,
        default="x64",
        help="Target architecture (default: x64)",
    )
    args = parser.parse_args()

    build_type: str = args.type
    generator: str = args.generator
    arch: str = args.arch
    gen_info = GENERATORS[generator]

    # ── Print configuration ──
    print("=" * 60)
    print("  Build Configuration")
    print(f"    BuildType:    {build_type}")
    print(f"    Generator:    {generator} ({gen_info['cmake']})")
    print(f"    Architecture: {arch}")
    print("=" * 60)

    # ── Setup environment ──
    env = None
    if not gen_info["ide"]:
        env = setup_msvc_env(arch)

    # ── Resolve vcpkg toolchain ──
    vcpkg_args = []
    vcpkg_root = os.environ.get("VCPKG_ROOT")
    if vcpkg_root:
        toolchain = Path(vcpkg_root) / "scripts" / "buildsystems" / "vcpkg.cmake"
        print(f"[INFO] Using vcpkg toolchain: {toolchain}")
        vcpkg_args = [f"-DCMAKE_TOOLCHAIN_FILE={toolchain}"]

    # ── Prepare build directory ──
    project_dir = Path(__file__).parent.resolve()
    build_dir = project_dir / "build" / build_type
    build_dir.mkdir(parents=True, exist_ok=True)

    # ── CMake configure ──
    print("\n[INFO] Configuring with CMake...")
    cmake_cmd = [
        "cmake",
        "-S", str(project_dir),
        "-B", str(build_dir),
        "-G", gen_info["cmake"],
        f"-DCMAKE_BUILD_TYPE={build_type}",
        *vcpkg_args,
    ]
    if gen_info["ide"]:
        cmake_cmd += ["-A", "x64" if arch == "x64" else "Win32"]

    run(cmake_cmd, env=env)

    # ── CMake build ──
    print("\n[INFO] Building...")
    run(["cmake", "--build", str(build_dir), "--config", build_type], env=env)

    print(f"\n{'=' * 60}")
    print(f"  Build completed: {build_type} | {generator} | {arch}")
    print(f"{'=' * 60}")


if __name__ == "__main__":
    main()
