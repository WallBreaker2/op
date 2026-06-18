#!/usr/bin/env python3
"""
Unified Build Script for op project.

Usage:
    python build.py [options]

Examples:
    python build.py                                # Release + latest supported VS + x64
    python build.py -t Debug                       # Debug + latest supported VS + x64
    python build.py -t Release -g nmake            # Release + NMake + x64
    python build.py -t RelWithDebInfo -g nmake     # RelWithDebInfo + NMake + x64
    python build.py -t Release -g vs2019           # Release + VS2019 + x64
    python build.py -t Release -a x86              # Release + VS2022 + x86
    python build.py -a x64 --deps-arch both        # Build for x64 but prepare dependencies for both x86 and x64
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
import urllib.request
import zipfile
from pathlib import Path

# ── Generator definitions ──────────────────────────────────────────

GENERATORS = {
    "nmake": {"cmake": "NMake Makefiles", "ide": False},
    "ninja": {"cmake": "Ninja", "ide": False},
    "vs2019": {"cmake": "Visual Studio 16 2019", "ide": True},
    "vs2022": {"cmake": "Visual Studio 17 2022", "ide": True},
    "vs2026": {"cmake": "Visual Studio 18 2026", "ide": True},
}

BUILD_TYPES = ["Debug", "Release", "RelWithDebInfo"]
ARCHITECTURES = ["x86", "x64"]
VCPKG_PACKAGES = ("gtest", "minhook")
VCPKG_TEST_STATIC_PACKAGES = ("gtest", "minhook")
OPENCV_REQUIRED_MODULES = (
    "core",
    "imgcodecs",
    "flann",
    "imgproc",
    "features",
    "objdetect",
    "stereo",
    "calib",
)
OPENCV_VERSION = "5.0.0"
OPENCV_LIB_SUFFIX = "".join(OPENCV_VERSION.split(".")[:3])
OPENCV_BUILD_SIG = json.dumps(
    {
        "modules": OPENCV_REQUIRED_MODULES,
        "runtime": "MT",
        "shared": False,
        "tag": OPENCV_VERSION,
        "with_ade": False,
        "with_jasper": False,
        "with_openexr": False,
        "with_openjpeg": False,
        "with_tiff": False,
        "with_webp": False,
    },
    sort_keys=True,
)
BLACKBONE_ASMJIT_STATIC_EXPORT_PATCH_SIG = "asmjit-static-no-dllexport-v3"
ARCH_TO_TRIPLET = {"x86": "x86-windows", "x64": "x64-windows"}
ARCH_TO_VS = {"x86": "Win32", "x64": "x64"}
BOOTSTRAP_STATE_FILE = ".deps-bootstrap-state.json"

# ── Helper functions ───────────────────────────────────────────────


def find_vswhere() -> Path | None:
    """Locate vswhere.exe if Visual Studio Installer is present."""
    program_files_x86 = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    vswhere = (
        Path(program_files_x86)
        / "Microsoft Visual Studio"
        / "Installer"
        / "vswhere.exe"
    )
    return vswhere if vswhere.is_file() else None


def find_latest_visual_studio_installation() -> Path | None:
    """Return the latest Visual Studio installation path, if available."""
    vswhere = find_vswhere()
    if vswhere is None:
        return None

    try:
        result = subprocess.run(
            [
                str(vswhere),
                "-latest",
                "-products",
                "*",
                "-requires",
                "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                "-property",
                "installationPath",
            ],
            capture_output=True,
            text=True,
            check=False,
        )
    except OSError:
        return None

    installation_path = result.stdout.strip()
    if not installation_path:
        return None

    path = Path(installation_path)
    return path.resolve() if path.exists() else None


def detect_supported_visual_studio_generator() -> str | None:
    """Map the latest installed Visual Studio to a supported generator key."""
    installation = find_latest_visual_studio_installation()
    if installation is None:
        return None

    version_hint = installation.parent.name.lower()
    version_map = {
        "2019": "vs2019",
        "2022": "vs2022",
        "18": "vs2026",
        "2026": "vs2026",
    }
    return version_map.get(version_hint)


def default_generator_key() -> str:
    """Pick the best IDE generator available on this machine."""
    return detect_supported_visual_studio_generator() or "vs2022"


def find_visual_studio_cmake() -> Path | None:
    """Locate the CMake bundled with Visual Studio when it is not on PATH."""
    direct = shutil.which("cmake")
    if direct:
        return Path(direct).resolve()

    installation = find_latest_visual_studio_installation()
    if installation is None:
        return None

    cmake = (
        installation
        / "Common7"
        / "IDE"
        / "CommonExtensions"
        / "Microsoft"
        / "CMake"
        / "CMake"
        / "bin"
        / "cmake.exe"
    )
    return cmake.resolve() if cmake.is_file() else None


def ensure_cmake_on_path() -> None:
    """Make the Visual Studio bundled CMake available to this process."""
    if shutil.which("cmake") is not None:
        return

    cmake = find_visual_studio_cmake()
    if cmake is None:
        return

    cmake_dir = str(cmake.parent)
    current_path = os.environ.get("PATH", "")
    os.environ["PATH"] = (
        cmake_dir if not current_path else cmake_dir + os.pathsep + current_path
    )
    print(f"[INFO] Using Visual Studio bundled CMake: {cmake}")


def find_vcvarsall() -> str | None:
    """Search common paths for vcvarsall.bat."""
    installation = find_latest_visual_studio_installation()
    if installation is not None:
        vcvarsall = installation / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat"
        if vcvarsall.is_file():
            return str(vcvarsall.resolve())

    candidates = [
        r"C:\Program Files\Microsoft Visual Studio\2022\{}\VC\Auxiliary\Build\vcvarsall.bat",
        r"D:\Program Files\Microsoft Visual Studio\18\{}\VC\Auxiliary\Build\vcvarsall.bat",
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


def download_file(url: str, destination: Path) -> None:
    """下载远程文件到本地目标路径。"""
    destination.parent.mkdir(parents=True, exist_ok=True)
    request = urllib.request.Request(
        url,
        headers={
            "Accept": "application/vnd.github+json",
            "User-Agent": "op-build-script/1.0",
        },
    )
    with urllib.request.urlopen(request) as response, destination.open("wb") as output:
        shutil.copyfileobj(response, output)


def get_opencv_release_info() -> tuple[str, str]:
    """返回项目固定使用的 OpenCV 版本号和源码 zip 地址。"""
    return (
        OPENCV_VERSION,
        f"https://github.com/opencv/opencv/archive/refs/tags/{OPENCV_VERSION}.zip",
    )


def ensure_opencv_source(
    opencv_root: Path, tag: str, archive_url: str
) -> tuple[Path, bool]:
    """确保指定版本的 OpenCV 源码已下载并解压。"""
    source_dir = opencv_root / f"opencv-{tag}"
    if (source_dir / "CMakeLists.txt").exists():
        return source_dir, False

    archive_path = opencv_root / f"opencv-{tag}.zip"
    if not archive_path.exists():
        print(f"[INFO] Downloading OpenCV {tag} source archive...")
        download_file(archive_url, archive_path)

    opencv_root.mkdir(parents=True, exist_ok=True)
    print(f"[INFO] Extracting OpenCV {tag}...")
    with zipfile.ZipFile(archive_path) as archive:
        archive.extractall(opencv_root)

    if not (source_dir / "CMakeLists.txt").exists():
        print(f"[ERROR] OpenCV source extraction failed: {source_dir}")
        sys.exit(1)

    return source_dir, True


def missing_opencv_install_items(install_root: Path, arch: str) -> list[str]:
    """返回当前项目需要但 OpenCV 安装目录中缺失的头文件或静态库。"""
    include_header = install_root / "include" / "opencv2" / "core.hpp"
    opencv_arch_dir = "x64" if arch == "x64" else "x86"
    staticlib_dirs = [
        install_root / opencv_arch_dir / vc_dir / "staticlib"
        for vc_dir in ("vc18", "vc17", "vc16")
    ]
    required_libs = (
        f"opencv_core{OPENCV_LIB_SUFFIX}",
        f"opencv_imgproc{OPENCV_LIB_SUFFIX}",
        f"opencv_imgcodecs{OPENCV_LIB_SUFFIX}",
        f"opencv_features{OPENCV_LIB_SUFFIX}",
        f"opencv_flann{OPENCV_LIB_SUFFIX}",
        f"opencv_geometry{OPENCV_LIB_SUFFIX}",
        f"opencv_objdetect{OPENCV_LIB_SUFFIX}",
        f"opencv_stereo{OPENCV_LIB_SUFFIX}",
        f"opencv_calib{OPENCV_LIB_SUFFIX}",
        "libjpeg-turbo",
        "libpng",
        "zlib",
        "libclapack",
    )

    def has_static_lib(stem: str) -> bool:
        return any(
            (staticlib_dir / f"{stem}.lib").exists()
            or (staticlib_dir / f"lib{stem}.a").exists()
            or (staticlib_dir / f"{stem}.a").exists()
            for staticlib_dir in staticlib_dirs
        )

    missing: list[str] = []
    if not include_header.exists():
        missing.append(str(include_header))
    if not any(staticlib_dir.exists() for staticlib_dir in staticlib_dirs):
        missing.append(
            "one of: "
            + ", ".join(str(staticlib_dir) for staticlib_dir in staticlib_dirs)
        )
    for name in required_libs:
        if not has_static_lib(name):
            missing.append(
                f"{name} (.lib/.a) in one of: "
                + ", ".join(str(staticlib_dir) for staticlib_dir in staticlib_dirs)
            )
    return missing


def has_opencv_install_layout(install_root: Path, arch: str) -> bool:
    """检查 OpenCV 安装目录是否包含当前项目需要的头文件和静态库。"""
    return not missing_opencv_install_items(install_root, arch)


def find_blackbone_lib(blackbone_root: Path, vs_arch: str) -> Path | None:
    build_root = blackbone_root / "build"
    search_roots = sorted(
        [path for path in build_root.glob(f"*-{vs_arch}") if path.is_dir()],
        reverse=True,
    )
    fallback_root = build_root / vs_arch
    if fallback_root.exists():
        search_roots.append(fallback_root)
    if not search_roots:
        return None

    libs: list[Path] = []
    for search_root in search_roots:
        libs.extend(sorted(search_root.rglob("BlackBone.lib")))
    if not libs:
        return None

    for lib in libs:
        if "release" in str(lib).lower():
            return lib.resolve()
    return libs[0].resolve()


def resolve_vcpkg_root(
    project_dir: Path, deps_dir: Path, vcpkg_root_arg: str | None
) -> Path:
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
    static_test_triplets: list[str],
    state: dict,
) -> bool:
    requested_pkg_sig = ",".join(VCPKG_PACKAGES)
    requested_static_sig = ",".join(VCPKG_TEST_STATIC_PACKAGES)

    if state.get("vcpkg_packages_sig") != requested_pkg_sig:
        state["vcpkg_packages_sig"] = requested_pkg_sig
        state["vcpkg_triplets"] = []

    if state.get("vcpkg_static_test_packages_sig") != requested_static_sig:
        state["vcpkg_static_test_packages_sig"] = requested_static_sig
        state["vcpkg_static_test_triplets"] = []

    installed_triplets = set(state.get("vcpkg_triplets", []))
    installed_static_triplets = set(state.get("vcpkg_static_test_triplets", []))
    pending = [triplet for triplet in triplets if triplet not in installed_triplets]
    pending_static = [
        triplet
        for triplet in static_test_triplets
        if triplet not in installed_static_triplets
    ]
    if not pending and not pending_static:
        print(f"[INFO] vcpkg dependencies already prepared for: {', '.join(triplets)}")
        if static_test_triplets:
            print(
                f"[INFO] Static test triplets already prepared for: {', '.join(static_test_triplets)}"
            )
        return False

    if not overlay_dir.exists():
        print(f"[ERROR] Overlay triplets directory not found: {overlay_dir}")
        sys.exit(1)

    for triplet in pending:
        pkg_args = [f"{pkg}:{triplet}" for pkg in VCPKG_PACKAGES]
        run(
            [
                str(vcpkg_exe),
                "install",
                *pkg_args,
                f"--overlay-triplets={overlay_dir}",
            ]
        )

    for triplet in pending_static:
        pkg_args = [f"{pkg}:{triplet}" for pkg in VCPKG_TEST_STATIC_PACKAGES]
        run(
            [
                str(vcpkg_exe),
                "install",
                *pkg_args,
            ]
        )

    state["vcpkg_triplets"] = sorted(installed_triplets.union(pending))
    state["vcpkg_static_test_triplets"] = sorted(
        installed_static_triplets.union(pending_static)
    )
    return True


def ensure_blackbone_repo(blackbone_root: Path) -> None:
    if blackbone_root.exists() and (blackbone_root / ".git").exists():
        return

    if blackbone_root.exists() and any(blackbone_root.iterdir()):
        print(
            f"[ERROR] BlackBone directory exists but is not a git repository: {blackbone_root}"
        )
        print("        Please clear it or pass a different --deps-dir path.")
        sys.exit(1)

    blackbone_root.parent.mkdir(parents=True, exist_ok=True)
    run(["git", "clone", "https://github.com/DarthTon/Blackbone", str(blackbone_root)])


def patch_blackbone_asmjit_static_exports(blackbone_root: Path) -> bool:
    """Prevent vendored AsmJit objects from forcing C++ exports into our DLLs."""
    cmake_file = blackbone_root / "src" / "BlackBone" / "CMakeLists.txt"
    if not cmake_file.exists():
        print(f"[ERROR] BlackBone CMakeLists.txt not found: {cmake_file}")
        sys.exit(1)

    changed = False
    cmake_text = cmake_file.read_text(encoding="utf-8")
    old_compile_def = (
        "target_compile_definitions(BlackBone PRIVATE BLACKBONE_STATIC ASMJIT_STATIC)"
    )
    compile_def = "target_compile_definitions(BlackBone PRIVATE BLACKBONE_STATIC)"
    if old_compile_def in cmake_text:
        cmake_text = cmake_text.replace(old_compile_def, compile_def, 1)
        cmake_file.write_text(cmake_text, encoding="utf-8")
        changed = True
    if compile_def not in cmake_text:
        cmake_text = cmake_text.replace(
            "add_library(BlackBone STATIC ${SOURCE_LIB} ${HEADER_LIB})",
            "add_library(BlackBone STATIC ${SOURCE_LIB} ${HEADER_LIB})\n"
            f"{compile_def}",
            1,
        )
        cmake_file.write_text(cmake_text, encoding="utf-8")
        changed = True

    asmjit_root = blackbone_root / "src" / "3rd_party" / "AsmJit"
    if not asmjit_root.exists():
        print(f"[ERROR] AsmJit source directory not found: {asmjit_root}")
        sys.exit(1)

    needle = "#define ASMJIT_EXPORTS"
    guarded_replacement = (
        "#if !defined(ASMJIT_STATIC) && !defined(BLACKBONE_STATIC)\n"
        "#define ASMJIT_EXPORTS\n"
        "#endif"
    )
    old_replacement = "#if !defined(ASMJIT_STATIC)\n#define ASMJIT_EXPORTS\n#endif"
    for source in sorted(asmjit_root.rglob("*.cpp")):
        text = source.read_text(encoding="utf-8")
        updated = text.replace(guarded_replacement, needle, 1)
        if updated == text:
            updated = text.replace(old_replacement, needle, 1)
        if updated == text:
            continue
        if updated != text:
            source.write_text(updated, encoding="utf-8")
            changed = True

    return changed


def ensure_blackbone_builds(
    blackbone_root: Path,
    generator_key: str,
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

    blackbone_patch_changed = patch_blackbone_asmjit_static_exports(blackbone_root)
    if (
        blackbone_patch_changed
        or state.get("blackbone_asmjit_patch_sig")
        != BLACKBONE_ASMJIT_STATIC_EXPORT_PATCH_SIG
    ):
        built_arches.clear()
        state["blackbone_asmjit_patch_sig"] = (
            BLACKBONE_ASMJIT_STATIC_EXPORT_PATCH_SIG
        )
        changed = True

    for arch in dep_arches:
        vs_arch = ARCH_TO_VS[arch]
        state_key = f"{generator_key}:{arch}"
        lib = find_blackbone_lib(blackbone_root, vs_arch)
        if state_key in built_arches and lib is not None:
            libs[arch] = lib
            continue

        build_dir = blackbone_root / "build" / f"{generator_key}-{vs_arch}"
        run(
            [
                "cmake",
                "-S",
                str(src_dir),
                "-B",
                str(build_dir),
                "-G",
                vs_generator,
                "-A",
                vs_arch,
                "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW",
                "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded",
            ]
        )
        run(["cmake", "--build", str(build_dir), "--config", "Release"])

        lib = find_blackbone_lib(blackbone_root, vs_arch)
        if lib is None:
            print(f"[ERROR] BlackBone.lib not found under: {build_dir}")
            sys.exit(1)

        libs[arch] = lib
        built_arches.add(state_key)
        changed = True

    state["blackbone_arches"] = sorted(built_arches)
    return libs, changed


def ensure_opencv_builds(
    opencv_source_dir: Path,
    opencv_root: Path,
    opencv_tag: str,
    generator_key: str,
    vs_generator: str,
    dep_arches: list[str],
    build_type: str,
    state: dict,
) -> tuple[dict[str, Path], bool]:
    """按最小模块集为各目标架构构建并安装静态 /MT 版 OpenCV。"""
    configured_arches = set(state.get("opencv_configured_arches", []))
    installed_configs = set(state.get("opencv_installed_configs", []))
    install_roots: dict[str, Path] = {}
    changed = False

    for arch in dep_arches:
        vs_arch = ARCH_TO_VS[arch]
        configure_key = f"{opencv_tag}:{generator_key}:{arch}"
        install_key = f"{opencv_tag}:{generator_key}:{arch}:{build_type}"
        build_dir = opencv_root / "build" / f"{generator_key}-{vs_arch}"
        install_root = opencv_root / "install" / f"{generator_key}-{vs_arch}"
        install_ready = has_opencv_install_layout(install_root, arch)

        needs_configure = (
            configure_key not in configured_arches
            or not (build_dir / "CMakeCache.txt").exists()
        )
        reused_existing = (
            not needs_configure and install_key in installed_configs and install_ready
        )
        if reused_existing:
            print(f"[INFO] Reusing existing OpenCV build for {arch}: {install_root}")

        if needs_configure:
            build_dir.mkdir(parents=True, exist_ok=True)
            run(
                [
                    "cmake",
                    "-S",
                    str(opencv_source_dir),
                    "-B",
                    str(build_dir),
                    "-G",
                    vs_generator,
                    "-A",
                    vs_arch,
                    "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW",
                    r"-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>",
                    f"-DCMAKE_INSTALL_PREFIX={install_root}",
                    "-DBUILD_SHARED_LIBS=OFF",
                    "-DBUILD_WITH_STATIC_CRT=ON",
                    f"-DBUILD_LIST={','.join(OPENCV_REQUIRED_MODULES)}",
                    "-DBUILD_opencv_world=OFF",
                    "-DBUILD_TESTS=OFF",
                    "-DBUILD_PERF_TESTS=OFF",
                    "-DBUILD_EXAMPLES=OFF",
                    "-DBUILD_DOCS=OFF",
                    "-DBUILD_PACKAGE=OFF",
                    "-DBUILD_JAVA=OFF",
                    "-DBUILD_opencv_apps=OFF",
                    "-DBUILD_opencv_gapi=OFF",
                    "-DBUILD_opencv_js=OFF",
                    "-DBUILD_opencv_python_bindings_generator=OFF",
                    "-DBUILD_opencv_python2=OFF",
                    "-DBUILD_opencv_python3=OFF",
                    "-DBUILD_PNG=ON",
                    "-DBUILD_JPEG=ON",
                    "-DBUILD_ZLIB=ON",
                    "-DWITH_JASPER=OFF",
                    "-DWITH_ADE=OFF",
                    "-DWITH_WEBP=OFF",
                    "-DWITH_TIFF=OFF",
                    "-DWITH_OPENJPEG=OFF",
                    "-DWITH_OPENEXR=OFF",
                    "-DBUILD_JASPER=OFF",
                    "-DBUILD_TIFF=OFF",
                    "-DBUILD_WEBP=OFF",
                    "-DBUILD_OPENJPEG=OFF",
                    "-DBUILD_OPENEXR=OFF",
                    "-DOPENCV_IO_ENABLE_OPENEXR=OFF",
                    "-DWITH_IPP=OFF",
                    "-DWITH_ITT=OFF",
                    "-DWITH_OPENCL=OFF",
                    "-DWITH_TBB=OFF",
                    "-DWITH_OPENMP=OFF",
                    "-DWITH_FFMPEG=OFF",
                    "-DWITH_GSTREAMER=OFF",
                    "-DWITH_MSMF=OFF",
                ]
            )
            configured_arches.add(configure_key)
            changed = True

        if install_key not in installed_configs or not install_ready:
            run(
                [
                    "cmake",
                    "--build",
                    str(build_dir),
                    "--config",
                    build_type,
                    "--target",
                    "install",
                ]
            )
            install_ready = has_opencv_install_layout(install_root, arch)
            if not install_ready:
                print(
                    f"[ERROR] OpenCV install layout is incomplete after install: {install_root}"
                )
                for missing_item in missing_opencv_install_items(install_root, arch):
                    print(f"        missing: {missing_item}")
                sys.exit(1)
            installed_configs.add(install_key)
            changed = True

        if not install_ready:
            print(f"[ERROR] OpenCV install layout is incomplete: {install_root}")
            for missing_item in missing_opencv_install_items(install_root, arch):
                print(f"        missing: {missing_item}")
            sys.exit(1)
        install_roots[arch] = install_root.resolve()

    state["opencv_configured_arches"] = sorted(configured_arches)
    state["opencv_installed_configs"] = sorted(installed_configs)
    return install_roots, changed


def bootstrap_dependencies(
    project_dir: Path,
    deps_dir: Path,
    dep_arches: list[str],
    generator_key: str,
    vs_generator: str,
    build_type: str,
    vcpkg_root_arg: str | None,
) -> tuple[Path, Path, dict[str, Path], dict[str, Path]]:
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
    static_test_triplets = [f"{triplet}-static" for triplet in triplets]
    vcpkg_changed = ensure_vcpkg_packages(
        vcpkg_exe, overlay_dir, triplets, static_test_triplets, state
    )

    blackbone_root = deps_dir / "BlackBone"
    ensure_blackbone_repo(blackbone_root)
    blackbone_libs, blackbone_changed = ensure_blackbone_builds(
        blackbone_root,
        generator_key,
        vs_generator,
        dep_arches,
        state,
    )

    opencv_root = deps_dir / "opencv"
    opencv_tag, opencv_archive_url = get_opencv_release_info()
    opencv_source_dir, opencv_source_changed = ensure_opencv_source(
        opencv_root, opencv_tag, opencv_archive_url
    )
    if state.get("opencv_build_sig") != OPENCV_BUILD_SIG:
        state["opencv_build_sig"] = OPENCV_BUILD_SIG
        state.pop("opencv_configured_arches", None)
        state.pop("opencv_installed_configs", None)
        state_changed = True

    opencv_install_roots, opencv_changed = ensure_opencv_builds(
        opencv_source_dir=opencv_source_dir,
        opencv_root=opencv_root,
        opencv_tag=opencv_tag,
        generator_key=generator_key,
        vs_generator=vs_generator,
        dep_arches=dep_arches,
        build_type=build_type,
        state=state,
    )

    if (
        state_changed
        or vcpkg_changed
        or blackbone_changed
        or opencv_source_changed
        or opencv_changed
        or not state_file.exists()
    ):
        save_bootstrap_state(state_file, state)

    return vcpkg_root, blackbone_root, blackbone_libs, opencv_install_roots


# ── Main ───────────────────────────────────────────────────────────


def main():
    detected_default_generator = default_generator_key()
    parser = argparse.ArgumentParser(
        description="Unified build script for the op project.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
examples:
  python build.py                                # Release + latest supported VS + x64
  python build.py -t Debug                       # Debug + latest supported VS + x64
  python build.py -t Release -g nmake            # Release + NMake + x64
  python build.py -t RelWithDebInfo -g vs2019    # RelWithDebInfo + VS2019 + x64
  python build.py -t Release -g vs2026           # Release + VS2026 + x64
  python build.py -t Release -a x86              # Release + default generator + x86
""",
    )
    parser.add_argument(
        "-t",
        "--type",
        choices=BUILD_TYPES,
        default="Release",
        help="Build type (default: Release)",
    )
    parser.add_argument(
        "-g",
        "--generator",
        choices=GENERATORS.keys(),
        default=detected_default_generator,
        help=f"CMake generator (default: {detected_default_generator})",
    )
    parser.add_argument(
        "-a",
        "--arch",
        choices=ARCHITECTURES,
        default="x64",
        help="Target architecture (default: x64)",
    )
    parser.add_argument(
        "--no-bootstrap-deps",
        action="store_true",
        help="Skip dependency bootstrap (vcpkg + BlackBone + OpenCV)",
    )
    parser.add_argument(
        "--deps-dir",
        default="build/_deps",
        help="Dependency cache directory (default: build/_deps)",
    )
    parser.add_argument(
        "--deps-arch",
        choices=["both", "x86", "x64"],
        default=None,
        help="Architectures to prepare dependencies for (default: follow --arch)",
    )
    parser.add_argument(
        "--vcpkg-root",
        default=None,
        help="Use an existing vcpkg root directory",
    )
    args = parser.parse_args()
    ensure_cmake_on_path()

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
    if args.deps_arch == "both":
        dep_arches = ["x86", "x64"]
    elif args.deps_arch in ("x86", "x64"):
        dep_arches = [args.deps_arch]
    else:
        dep_arches = [arch]

    if arch not in dep_arches:
        dep_arches.append(arch)

    vcpkg_root: Path | None = None
    blackbone_root: Path | None = None
    blackbone_libs: dict[str, Path] = {}
    opencv_install_roots: dict[str, Path] = {}
    if not args.no_bootstrap_deps:
        dep_vs_generator_key = (
            generator if generator.startswith("vs") else default_generator_key()
        )
        dep_vs_generator = GENERATORS[dep_vs_generator_key]["cmake"]
        print("\n[INFO] Bootstrapping third-party dependencies...")
        vcpkg_root, blackbone_root, blackbone_libs, opencv_install_roots = (
            bootstrap_dependencies(
                project_dir=project_dir,
                deps_dir=deps_dir,
                dep_arches=dep_arches,
                generator_key=dep_vs_generator_key,
                vs_generator=dep_vs_generator,
                build_type=build_type,
                vcpkg_root_arg=args.vcpkg_root,
            )
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
    selected_opencv_root = opencv_install_roots.get(arch)
    if selected_opencv_root:
        env["OPENCV_ROOT"] = str(selected_opencv_root)

    # ── Resolve dependency-related configure args ──
    vcpkg_args = []
    env_vcpkg_root = env.get("VCPKG_ROOT")
    if env_vcpkg_root:
        print(f"[INFO] Using vcpkg root: {env_vcpkg_root}")

    blackbone_args: list[str] = []
    if blackbone_root:
        blackbone_args.append(f"-DBLACKBONE_ROOT={blackbone_root}")

    selected_blackbone_lib = blackbone_libs.get(arch)
    if selected_blackbone_lib and blackbone_root is not None:
        blackbone_include = blackbone_root / "src"
        blackbone_args.extend(
            [
                f"-DBLACKBONE_INCLUDE_DIR={blackbone_include}",
                f"-DBLACKBONE_LIBRARY={selected_blackbone_lib}",
            ]
        )

    opencv_args: list[str] = []
    if selected_opencv_root:
        print(f"[INFO] Using OpenCV root: {selected_opencv_root}")
        opencv_args.append(f"-DOPENCV_ROOT={selected_opencv_root}")
        opencv_args.append(f"-DOPENCV_LIB_SUFFIX={OPENCV_LIB_SUFFIX}")

    # ── Prepare build directory ──
    # Keep architecture/generator isolated to avoid CMake cache platform clashes
    # (e.g. Win32 vs x64 in the same binary dir).
    build_dir_name = f"{generator}-{arch}-{build_type}"
    build_dir = project_dir / "build" / build_dir_name
    build_dir.mkdir(parents=True, exist_ok=True)

    # ── CMake configure ──
    print("\n[INFO] Configuring with CMake...")
    cmake_cmd = [
        "cmake",
        "-S",
        str(project_dir),
        "-B",
        str(build_dir),
        "-G",
        gen_info["cmake"],
        f"-DCMAKE_BUILD_TYPE={build_type}",
        *vcpkg_args,
        *blackbone_args,
        *opencv_args,
    ]
    if gen_info["ide"]:
        cmake_cmd += ["-A", "x64" if arch == "x64" else "Win32"]

    run(cmake_cmd, env=env)

    # ── CMake build ──
    print("\n[INFO] Building...")
    run(["cmake", "--build", str(build_dir), "--config", build_type], env=env)

    if build_type == "Release":
        print("\n[INFO] Installing Release artifacts...")
        run(
            ["cmake", "--build", str(build_dir), "--config", build_type, "--target", "install"],
            env=env,
        )

    print(f"\n{'=' * 60}")
    print(f"  Build completed: {build_type} | {generator} | {arch}")
    print(f"  Build directory: {build_dir}")
    if build_type == "Release":
        print(f"  Install output:   {project_dir / 'bin' / arch}")
    print(f"{'=' * 60}")


if __name__ == "__main__":
    main()
