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
import json
import os
import shutil
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
VCPKG_PACKAGES = ("gtest", "minhook")
ARCH_TO_TRIPLET = {"x86": "x86-windows", "x64": "x64-windows"}
ARCH_TO_VS = {"x86": "Win32", "x64": "x64"}
BOOTSTRAP_STATE_FILE = ".deps-bootstrap-state.json"

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


def check_required_tools() -> None:
    """Ensure required CLI tools are available."""
    for tool in ("git", "cmake"):
        if shutil.which(tool) is None:
            print(f"[ERROR] Required tool not found in PATH: {tool}")
            sys.exit(1)


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


def run(
    cmd: list[str] | str,
    env: dict[str, str] | None = None,
    cwd: Path | None = None,
    shell: bool = False,
) -> None:
    """Run a command, printing it first, and exit on failure."""
    if isinstance(cmd, list):
        cmd = [str(part) for part in cmd]
        display = " ".join(cmd)
    else:
        display = cmd
    print(f"[RUN] {display}\n")
    result = subprocess.run(cmd, env=env, cwd=cwd, shell=shell)
    if result.returncode != 0:
        print(f"\n[ERROR] Command failed with exit code {result.returncode}")
        sys.exit(result.returncode)


def resolve_path(project_dir: Path, path_arg: str) -> Path:
    path = Path(path_arg)
    if path.is_absolute():
        return path
    return (project_dir / path).resolve()


def load_bootstrap_state(state_file: Path) -> dict:
    if not state_file.exists():
        return {}
    try:
        return json.loads(state_file.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}


def save_bootstrap_state(state_file: Path, state: dict) -> None:
    state_file.parent.mkdir(parents=True, exist_ok=True)
    state_file.write_text(json.dumps(state, indent=2), encoding="utf-8")


def find_blackbone_lib(blackbone_root: Path, vs_arch: str) -> Path | None:
    search_root = blackbone_root / "build" / vs_arch
    if not search_root.exists():
        return None

    libs = sorted(search_root.rglob("BlackBone.lib"))
    if not libs:
        return None

    for lib in libs:
        if "release" in str(lib).lower():
            return lib.resolve()
    return libs[0].resolve()


def resolve_vcpkg_root(project_dir: Path, deps_dir: Path, vcpkg_root_arg: str | None) -> Path:
    candidates: list[Path] = []
    if vcpkg_root_arg:
        candidates.append(resolve_path(project_dir, vcpkg_root_arg))

    env_root = os.environ.get("VCPKG_ROOT")
    if env_root:
        candidates.append(Path(env_root).resolve())

    userprofile = os.environ.get("USERPROFILE")
    if userprofile:
        candidates.append((Path(userprofile) / "vcpkg").resolve())

    candidates.append((deps_dir / "vcpkg").resolve())

    for candidate in candidates:
        if candidate.exists():
            return candidate

    return (deps_dir / "vcpkg").resolve()


def ensure_vcpkg(vcpkg_root: Path) -> Path:
    if not vcpkg_root.exists():
        vcpkg_root.parent.mkdir(parents=True, exist_ok=True)
        run(["git", "clone", "https://github.com/microsoft/vcpkg.git", str(vcpkg_root)])

    vcpkg_exe = vcpkg_root / "vcpkg.exe"
    if not vcpkg_exe.exists():
        run("bootstrap-vcpkg.bat", cwd=vcpkg_root, shell=True)

    if not vcpkg_exe.exists():
        print(f"[ERROR] vcpkg bootstrap failed: {vcpkg_exe} not found")
        sys.exit(1)

    return vcpkg_exe


def ensure_vcpkg_packages(
    vcpkg_exe: Path,
    overlay_dir: Path,
    triplets: list[str],
    state: dict,
) -> bool:
    installed_triplets = set(state.get("vcpkg_triplets", []))
    pending = [triplet for triplet in triplets if triplet not in installed_triplets]
    if not pending:
        print(f"[INFO] vcpkg dependencies already prepared for: {', '.join(triplets)}")
        return False

    if not overlay_dir.exists():
        print(f"[ERROR] Overlay triplets directory not found: {overlay_dir}")
        sys.exit(1)

    for triplet in pending:
        pkg_args = [f"{pkg}:{triplet}" for pkg in VCPKG_PACKAGES]
        run([
            str(vcpkg_exe),
            "install",
            *pkg_args,
            f"--overlay-triplets={overlay_dir}",
        ])

    state["vcpkg_triplets"] = sorted(installed_triplets.union(pending))
    return True


def ensure_blackbone_repo(blackbone_root: Path) -> None:
    if blackbone_root.exists() and (blackbone_root / ".git").exists():
        return

    if blackbone_root.exists() and any(blackbone_root.iterdir()):
        print(f"[ERROR] BlackBone directory exists but is not a git repository: {blackbone_root}")
        print("        Please clear it or pass a different --deps-dir path.")
        sys.exit(1)

    blackbone_root.parent.mkdir(parents=True, exist_ok=True)
    run(["git", "clone", "https://github.com/DarthTon/Blackbone", str(blackbone_root)])


def ensure_blackbone_builds(
    blackbone_root: Path,
    vs_generator: str,
    dep_arches: list[str],
    state: dict,
) -> tuple[dict[str, Path], bool]:
    built_arches = set(state.get("blackbone_arches", []))
    libs: dict[str, Path] = {}
    changed = False

    src_dir = blackbone_root / "src"
    if not src_dir.exists():
        print(f"[ERROR] BlackBone source directory not found: {src_dir}")
        sys.exit(1)

    for arch in dep_arches:
        vs_arch = ARCH_TO_VS[arch]
        lib = find_blackbone_lib(blackbone_root, vs_arch)
        if arch in built_arches and lib is not None:
            libs[arch] = lib
            continue

        build_dir = blackbone_root / "build" / vs_arch
        run([
            "cmake",
            "-S", str(src_dir),
            "-B", str(build_dir),
            "-G", vs_generator,
            "-A", vs_arch,
            "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW",
            "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded",
        ])
        run(["cmake", "--build", str(build_dir), "--config", "Release"])

        lib = find_blackbone_lib(blackbone_root, vs_arch)
        if lib is None:
            print(f"[ERROR] BlackBone.lib not found under: {build_dir}")
            sys.exit(1)

        libs[arch] = lib
        built_arches.add(arch)
        changed = True

    state["blackbone_arches"] = sorted(built_arches)
    return libs, changed


def bootstrap_dependencies(
    project_dir: Path,
    deps_dir: Path,
    dep_arches: list[str],
    vs_generator: str,
    vcpkg_root_arg: str | None,
) -> tuple[Path, Path, dict[str, Path]]:
    check_required_tools()

    deps_dir.mkdir(parents=True, exist_ok=True)
    state_file = deps_dir / BOOTSTRAP_STATE_FILE
    state = load_bootstrap_state(state_file)
    state_changed = False

    vcpkg_root = resolve_vcpkg_root(project_dir, deps_dir, vcpkg_root_arg)
    previous_vcpkg_root = state.get("vcpkg_root")
    if previous_vcpkg_root and Path(previous_vcpkg_root).resolve() != vcpkg_root:
        state.pop("vcpkg_triplets", None)
        state_changed = True

    vcpkg_exe = ensure_vcpkg(vcpkg_root)
    if state.get("vcpkg_root") != str(vcpkg_root):
        state["vcpkg_root"] = str(vcpkg_root)
        state_changed = True

    overlay_dir = project_dir / "ci" / "triplets"
    triplets = [ARCH_TO_TRIPLET[arch] for arch in dep_arches]
    vcpkg_changed = ensure_vcpkg_packages(vcpkg_exe, overlay_dir, triplets, state)

    blackbone_root = deps_dir / "BlackBone"
    ensure_blackbone_repo(blackbone_root)
    blackbone_libs, blackbone_changed = ensure_blackbone_builds(
        blackbone_root,
        vs_generator,
        dep_arches,
        state,
    )

    if state_changed or vcpkg_changed or blackbone_changed or not state_file.exists():
        save_bootstrap_state(state_file, state)

    return vcpkg_root, blackbone_root, blackbone_libs


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
    parser.add_argument(
        "--no-bootstrap-deps",
        action="store_true",
        help="Skip dependency bootstrap (vcpkg + BlackBone)",
    )
    parser.add_argument(
        "--deps-dir",
        default="build/_deps",
        help="Dependency cache directory (default: build/_deps)",
    )
    parser.add_argument(
        "--deps-arch",
        choices=["both", "x86", "x64"],
        default="both",
        help="Architectures to prepare dependencies for (default: both)",
    )
    parser.add_argument(
        "--vcpkg-root",
        default=None,
        help="Use an existing vcpkg root directory",
    )
    args = parser.parse_args()

    build_type: str = args.type
    generator: str = args.generator
    arch: str = args.arch
    gen_info = GENERATORS[generator]
    project_dir = Path(__file__).parent.resolve()
    deps_dir = resolve_path(project_dir, args.deps_dir)

    # ── Print configuration ──
    print("=" * 60)
    print("  Build Configuration")
    print(f"    BuildType:    {build_type}")
    print(f"    Generator:    {generator} ({gen_info['cmake']})")
    print(f"    Architecture: {arch}")
    print(f"    BootstrapDeps:{'No' if args.no_bootstrap_deps else 'Yes'}")
    print("=" * 60)

    # ── Bootstrap dependencies ──
    dep_arches = ["x86", "x64"] if args.deps_arch == "both" else [args.deps_arch]
    if arch not in dep_arches:
        dep_arches.append(arch)

    vcpkg_root: Path | None = None
    blackbone_root: Path | None = None
    blackbone_libs: dict[str, Path] = {}
    if not args.no_bootstrap_deps:
        dep_vs_generator = GENERATORS[generator]["cmake"] if generator.startswith("vs") else GENERATORS["vs2022"]["cmake"]
        print("\n[INFO] Bootstrapping third-party dependencies...")
        vcpkg_root, blackbone_root, blackbone_libs = bootstrap_dependencies(
            project_dir=project_dir,
            deps_dir=deps_dir,
            dep_arches=dep_arches,
            vs_generator=dep_vs_generator,
            vcpkg_root_arg=args.vcpkg_root,
        )
    elif args.vcpkg_root:
        vcpkg_root = resolve_path(project_dir, args.vcpkg_root)

    # ── Setup environment ──
    env = os.environ.copy()
    if not gen_info["ide"]:
        env = setup_msvc_env(arch)

    if vcpkg_root:
        env["VCPKG_ROOT"] = str(vcpkg_root)
    if blackbone_root:
        env["BLACKBONE_ROOT"] = str(blackbone_root)

    # ── Resolve vcpkg toolchain ──
    vcpkg_args = []
    env_vcpkg_root = env.get("VCPKG_ROOT")
    if env_vcpkg_root:
        toolchain = Path(env_vcpkg_root) / "scripts" / "buildsystems" / "vcpkg.cmake"
        print(f"[INFO] Using vcpkg toolchain: {toolchain}")
        vcpkg_args = [f"-DCMAKE_TOOLCHAIN_FILE={toolchain}"]

        overlay_dir = project_dir / "ci" / "triplets"
        if overlay_dir.exists():
            vcpkg_args.append(f"-DVCPKG_OVERLAY_TRIPLETS={overlay_dir}")

    blackbone_args: list[str] = []
    if blackbone_root:
        blackbone_args.append(f"-DBLACKBONE_ROOT={blackbone_root}")

    selected_blackbone_lib = blackbone_libs.get(arch)
    if selected_blackbone_lib and blackbone_root is not None:
        blackbone_include = blackbone_root / "src"
        blackbone_args.extend([
            f"-DBLACKBONE_INCLUDE_DIR={blackbone_include}",
            f"-DBLACKBONE_LIBRARY={selected_blackbone_lib}",
        ])

    # ── Prepare build directory ──
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
        *blackbone_args,
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
