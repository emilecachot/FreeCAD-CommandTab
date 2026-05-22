#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_QT_MAJOR = os.environ.get("QT_MAJOR", "6")
DEFAULT_PACKAGE_DIR = ROOT / "dist" / "FreeCAD-CommandTab"
DEFAULT_CMAKE_EXECUTABLE = os.environ.get("CMAKE", "cmake")
DEFAULT_BUILD_CONFIG = "Release"

PLATFORM_ALIASES = {
    "darwin": "macos",
    "mac": "macos",
    "osx": "macos",
    "win": "windows",
    "windows": "windows",
    "linux": "linux",
}

MACOS_QT_LIBRARY_RELINKS = [
    ("/opt/homebrew/opt/qtsvg/lib/QtSvg.framework/Versions/A/QtSvg", "{freecad_lib}/libQt6Svg.6.dylib"),
    ("/opt/homebrew/opt/qtbase/lib/QtWidgets.framework/Versions/A/QtWidgets", "{freecad_lib}/libQt6Widgets.6.dylib"),
    ("/opt/homebrew/opt/qtbase/lib/QtGui.framework/Versions/A/QtGui", "{freecad_lib}/libQt6Gui.6.dylib"),
    ("/opt/homebrew/opt/qtbase/lib/QtCore.framework/Versions/A/QtCore", "{freecad_lib}/libQt6Core.6.dylib"),
    ("/usr/local/opt/qtsvg/lib/QtSvg.framework/Versions/A/QtSvg", "{freecad_lib}/libQt6Svg.6.dylib"),
    ("/usr/local/opt/qtbase/lib/QtWidgets.framework/Versions/A/QtWidgets", "{freecad_lib}/libQt6Widgets.6.dylib"),
    ("/usr/local/opt/qtbase/lib/QtGui.framework/Versions/A/QtGui", "{freecad_lib}/libQt6Gui.6.dylib"),
    ("/usr/local/opt/qtbase/lib/QtCore.framework/Versions/A/QtCore", "{freecad_lib}/libQt6Core.6.dylib"),
    ("@rpath/QtSvg.framework/Versions/A/QtSvg", "{freecad_lib}/libQt6Svg.6.dylib"),
    ("@rpath/QtWidgets.framework/Versions/A/QtWidgets", "{freecad_lib}/libQt6Widgets.6.dylib"),
    ("@rpath/QtGui.framework/Versions/A/QtGui", "{freecad_lib}/libQt6Gui.6.dylib"),
    ("@rpath/QtCore.framework/Versions/A/QtCore", "{freecad_lib}/libQt6Core.6.dylib"),
]


def detect_native_platform() -> str:
    system_name = platform.system()
    if system_name == "Windows":
        return "windows"
    if system_name == "Darwin":
        return "macos"
    if system_name == "Linux":
        return "linux"
    raise RuntimeError(f"Unsupported host platform: {system_name}")


def normalize_native_platform(value: str | None) -> str:
    if not value:
        value = os.environ.get("NATIVE_PLATFORM")
    if not value:
        return detect_native_platform()
    normalized = PLATFORM_ALIASES.get(value.strip().lower())
    if not normalized:
        raise argparse.ArgumentTypeError(
            f"unknown native platform: {value}. Expected linux, windows, or macos."
        )
    return normalized


def maybe_relink_macos_qt_dependencies(library_path: Path) -> None:
    freecad_lib_dir = Path(os.environ.get("FREECAD_LIB_DIR", "/Applications/FreeCAD.app/Contents/Resources/lib"))
    if platform.system() != "Darwin" or not library_path.exists() or not freecad_lib_dir.is_dir():
        return
    if shutil.which("install_name_tool") is None:
        return

    try:
        output = subprocess.check_output(["otool", "-L", str(library_path)], text=True)
    except subprocess.CalledProcessError:
        return

    for old, new_template in MACOS_QT_LIBRARY_RELINKS:
        if old in output:
            new_path = new_template.format(freecad_lib=str(freecad_lib_dir))
            if Path(new_path).exists():
                subprocess.run([
                    "install_name_tool",
                    "-change",
                    old,
                    new_path,
                    str(library_path),
                ], check=True)


def resolve_executable(value: str) -> str:
    candidate = Path(value)
    if candidate.parent != Path("") and candidate.exists():
        return str(candidate)
    resolved = shutil.which(value)
    if resolved:
        return resolved
    raise RuntimeError(
        f"Executable not found: {value}. Install CMake or set the CMAKE environment variable."
    )


def find_qmake_executable(qt_install_dir: Path | None = None) -> str | None:
    if qt_install_dir:
        candidates = [
            qt_install_dir / "bin" / "qmake.exe",
            qt_install_dir.parent / "qt6" / "bin" / "qmake.exe",
            qt_install_dir.parent / "bin" / "qmake.exe",
        ]
        for candidate in candidates:
            if candidate.exists():
                return str(candidate)
    return shutil.which("qmake") or shutil.which("qmake.exe")


def get_qmake_spec(qt_install_dir: Path | None = None) -> str | None:
    qmake = find_qmake_executable(qt_install_dir)
    if qmake is None:
        return None
    try:
        return subprocess.check_output([qmake, "-query", "QMAKE_SPEC"], text=True, stderr=subprocess.DEVNULL).strip()
    except subprocess.CalledProcessError:
        return None


def get_qmake_version(qt_install_dir: Path | None = None) -> str | None:
    qmake = find_qmake_executable(qt_install_dir)
    if qmake is None:
        return None
    try:
        return subprocess.check_output([qmake, "-query", "QT_VERSION"], text=True, stderr=subprocess.DEVNULL).strip()
    except subprocess.CalledProcessError:
        return None


def get_common_qt_roots() -> list[Path]:
    roots: list[Path] = []
    system_name = platform.system()
    if system_name == "Windows":
        roots.extend([
            Path("C:/Qt"),
            Path(os.environ.get("ProgramFiles", "")) / "Qt",
            Path(os.environ.get("ProgramFiles(x86)", "")) / "Qt",
            Path(os.environ.get("USERPROFILE", "")) / "Qt",
        ])
    elif system_name == "Darwin":
        roots.extend([
            Path(os.environ.get("HOME", "")) / "Qt",
            Path("/usr/local/opt/qt"),
            Path("/opt/homebrew/opt/qt"),
        ])
    else:
        roots.extend([
            Path(os.environ.get("HOME", "")) / "Qt",
            Path("/opt/qt"),
            Path("/usr/local/opt/qt"),
        ])
    return [root for root in roots if root.exists()]


def find_qt_install_dir(qt_major: str, explicit_path: Path | None = None) -> Path | None:
    if explicit_path is not None:
        if explicit_path.exists():
            return explicit_path
        return None

    candidate_roots = get_common_qt_roots()
    for root in candidate_roots:
        # search direct Qt version directories like C:/Qt/6.10.0/mingw_64
        for version_dir in sorted(root.glob(f"{qt_major}.*")):
            if version_dir.is_dir():
                for toolchain_dir in sorted(version_dir.iterdir()):
                    if (toolchain_dir / "bin" / "qmake.exe").exists():
                        return toolchain_dir
                if (version_dir / "bin" / "qmake.exe").exists():
                    return version_dir
        # search first-level toolchain directories under root like C:/Qt/6.10.0/mingw_64
        for toolchain_dir in sorted(root.glob(f"*{qt_major}*")):
            if toolchain_dir.is_dir() and (toolchain_dir / "bin" / "qmake.exe").exists():
                return toolchain_dir

    return None


def infer_qt_toolchain(qt_install_dir: Path | None = None) -> str | None:
    spec = get_qmake_spec(qt_install_dir)
    if spec:
        spec_lower = spec.lower()
        if "mingw" in spec_lower or "g++" in spec_lower:
            return "mingw"
        if "msvc" in spec_lower:
            return "msvc"
    if qt_install_dir:
        path_lower = str(qt_install_dir).lower()
        if "mingw" in path_lower:
            return "mingw"
        if "msvc" in path_lower or "vc" in path_lower:
            return "msvc"
    return None


def freecad_qt_source_root(qt_install_dir: Path) -> Path | None:
    if (qt_install_dir / "Qt6" / "Qt6Config.cmake").is_file():
        if qt_install_dir.name.lower() == "cmake" and qt_install_dir.parent.name.lower() == "lib":
            return qt_install_dir.parent.parent
    if (qt_install_dir / "lib" / "cmake" / "Qt6" / "Qt6Config.cmake").is_file():
        return qt_install_dir
    return None


def find_windows_msvc_qt_headers(qt_major: str = "6") -> Path | None:
    for root in get_common_qt_roots():
        for include_dir in sorted(root.glob(f"{qt_major}.*/*msvc*/include"), reverse=True):
            if (include_dir / "QtCore").is_dir():
                return include_dir
    return None


def _msvc_tool(env: dict[str, str], name: str) -> str:
    path_value = env.get("PATH", "")
    resolved = shutil.which(name, path=path_value)
    if resolved:
        return resolved
    raise RuntimeError(f"MSVC tool not found after vcvars initialization: {name}")


def _dumpbin_export_names(dumpbin_exe: str, dll_path: Path, env: dict[str, str]) -> list[str]:
    output = subprocess.check_output(
        [dumpbin_exe, "/nologo", "/exports", str(dll_path)],
        text=True,
        stderr=subprocess.DEVNULL,
        env=env,
    )
    exports: list[str] = []
    in_export_table = False
    for line in output.splitlines():
        stripped = line.strip()
        if stripped.startswith("ordinal hint RVA"):
            in_export_table = True
            continue
        if in_export_table is False:
            continue
        if stripped == "" or stripped.startswith("Summary"):
            continue
        parts = stripped.split()
        if len(parts) < 4 or parts[0].isdigit() is False:
            continue
        name = parts[3]
        if name and name not in exports:
            exports.append(name)
    return exports


def _write_def_file(def_path: Path, dll_name: str, exports: list[str]) -> None:
    lines = [f"LIBRARY {dll_name}", "EXPORTS"]
    lines.extend(f"    {name}" for name in exports)
    def_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def _generate_msvc_import_library(
    *,
    dll_path: Path,
    output_lib: Path,
    env: dict[str, str],
) -> None:
    if output_lib.is_file():
        return
    dumpbin_exe = _msvc_tool(env, "dumpbin.exe")
    lib_exe = _msvc_tool(env, "lib.exe")
    exports = _dumpbin_export_names(dumpbin_exe, dll_path, env)
    if not exports:
        raise RuntimeError(f"No exports found in {dll_path}")
    def_path = output_lib.with_suffix(".def")
    _write_def_file(def_path, dll_path.name, exports)
    subprocess.run(
        [
            lib_exe,
            "/nologo",
            f"/def:{def_path}",
            "/machine:x64",
            f"/out:{output_lib}",
        ],
        check=True,
        env=env,
    )


def _generate_empty_msvc_library(output_lib: Path, env: dict[str, str]) -> None:
    if output_lib.is_file():
        return
    lib_exe = _msvc_tool(env, "lib.exe")
    def_path = output_lib.with_suffix(".def")
    _write_def_file(def_path, output_lib.with_suffix(".dll").name, [])
    subprocess.run(
        [
            lib_exe,
            "/nologo",
            f"/def:{def_path}",
            "/machine:x64",
            f"/out:{output_lib}",
        ],
        check=True,
        env=env,
    )


def prepare_windows_msvc_qt_sdk(
    qt_install_dir: Path | None,
    *,
    env: dict[str, str] | None,
) -> Path | None:
    if platform.system() != "Windows" or qt_install_dir is None or env is None:
        return qt_install_dir

    source_root = freecad_qt_source_root(qt_install_dir)
    if source_root is None:
        return qt_install_dir

    source_bin = source_root / "lib" / "qt6" / "bin"
    source_cmake = source_root / "lib" / "cmake"
    if (source_bin / "Qt6Core.dll").is_file() is False:
        return qt_install_dir
    if (source_root / "lib" / "Qt6Core.lib").is_file():
        return qt_install_dir

    sdk_root = ROOT / "build" / "freecad-qt-msvc-sdk"
    sdk_lib = sdk_root / "lib"
    sdk_tools = sdk_lib / "qt6"
    sdk_bin = sdk_lib / "qt6" / "bin"
    sdk_cmake = sdk_lib / "cmake"
    sdk_include = sdk_root / "include" / "qt6"
    sdk_bin.mkdir(parents=True, exist_ok=True)
    sdk_tools.mkdir(parents=True, exist_ok=True)
    sdk_lib.mkdir(parents=True, exist_ok=True)
    shutil.copytree(source_cmake, sdk_cmake, dirs_exist_ok=True)
    source_include = source_root / "include" / "qt6"
    if source_include.is_dir() is False:
        source_include = find_windows_msvc_qt_headers("6") or source_include
    if source_include.is_dir():
        shutil.copytree(source_include, sdk_include, dirs_exist_ok=True)
    for tool_path in (source_root / "lib" / "qt6").glob("*.exe"):
        shutil.copy2(tool_path, sdk_tools / tool_path.name)
    for tool_path in source_bin.glob("*.exe"):
        shutil.copy2(tool_path, sdk_bin / tool_path.name)
    source_mkspecs = source_root / "lib" / "qt6" / "mkspecs"
    if source_mkspecs.is_dir():
        shutil.copytree(source_mkspecs, sdk_tools / "mkspecs", dirs_exist_ok=True)
    source_plugins = source_root / "lib" / "qt6" / "plugins"
    if source_plugins.is_dir():
        shutil.copytree(source_plugins, sdk_tools / "plugins", dirs_exist_ok=True)
    for prl_path in (source_root / "lib").glob("Qt6*.prl"):
        shutil.copy2(prl_path, sdk_lib / prl_path.name)
    for dll_path in source_bin.glob("Qt6*.dll"):
        shutil.copy2(dll_path, sdk_bin / dll_path.name)
        _generate_msvc_import_library(
            dll_path=dll_path,
            output_lib=sdk_lib / dll_path.with_suffix(".lib").name,
            env=env,
        )
    _generate_empty_msvc_library(sdk_lib / "Qt6EntryPoint.lib", env)
    env["PATH"] = os.pathsep.join(
        [
            str(sdk_bin),
            str(sdk_tools),
            str(source_bin),
            str(source_root / "bin"),
            env.get("PATH", ""),
        ]
    )
    return sdk_lib / "cmake"


def parse_cmake_generators(cmake_executable: str) -> set[str]:
    generators: set[str] = set()
    try:
        output = subprocess.check_output([cmake_executable, "--help"], text=True)
    except subprocess.CalledProcessError:
        return generators
    for line in output.splitlines():
        line = line.strip()
        if not line or "=" not in line:
            continue
        name = line.split("=", 1)[0].strip()
        if name:
            # Normalize generator names such as "Visual Studio 15 2017 [arch]".
            generators.add(name.replace(" [arch]", ""))
    return generators


def find_vswhere() -> Path | None:
    candidates = [
        Path(os.environ.get("ProgramFiles(x86)", "")) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe",
        Path(os.environ.get("ProgramFiles", "")) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return None


def find_visual_studio_installation() -> Path | None:
    vswhere = find_vswhere()
    if vswhere is None:
        return None
    try:
        output = subprocess.check_output(
            [str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"],
            text=True,
            stderr=subprocess.DEVNULL,
        )
    except subprocess.CalledProcessError:
        return None
    path = output.strip()
    return Path(path) if path else None


def find_vcvarsall(installation_path: Path) -> Path | None:
    candidate = installation_path / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat"
    return candidate if candidate.exists() else None


def get_visual_studio_env(arch: str = "x64") -> dict[str, str] | None:
    installation_path = find_visual_studio_installation()
    if installation_path is None:
        return None
    vcvarsall = find_vcvarsall(installation_path)
    if vcvarsall is None:
        return None

    command = f'call "{vcvarsall}" {arch} >nul && set'
    try:
        output = subprocess.check_output(command, shell=True, text=True, stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError:
        return None

    env: dict[str, str] = {}
    for line in output.splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        env[key] = value
    return env


def choose_default_generator(native_platform: str, cmake_executable: str, qt_toolchain: str | None = None) -> list[str] | None:
    if native_platform != "windows":
        return None

    if qt_toolchain == "mingw":
        if shutil.which("mingw32-make") and shutil.which("gcc"):
            return ["-G", "MinGW Makefiles"]

    if qt_toolchain == "msvc":
        if get_visual_studio_env() is not None:
            return ["-G", "NMake Makefiles"]

    if shutil.which("ninja"):
        return ["-G", "Ninja"]

    if get_visual_studio_env() is not None:
        return ["-G", "NMake Makefiles"]

    if shutil.which("mingw32-make") and shutil.which("gcc") and shutil.which("cl.exe") is None:
        return ["-G", "MinGW Makefiles"]

    available = parse_cmake_generators(cmake_executable)
    preferred = [
        "Visual Studio 17 2022",
        "Visual Studio 16 2019",
        "Visual Studio 15 2017",
        "NMake Makefiles JOM",
        "NMake Makefiles",
        "MinGW Makefiles",
    ]
    for generator in preferred:
        if generator in available:
            args = ["-G", generator]
            if generator.startswith("Visual Studio") and platform.machine().endswith("64"):
                args.extend(["-A", "x64"])
            return args

    return None


def get_cmake_cache_generator(build_dir: Path) -> str | None:
    cache_file = build_dir / "CMakeCache.txt"
    if not cache_file.exists():
        return None
    for line in cache_file.read_text().splitlines():
        if line.startswith("CMAKE_GENERATOR:INTERNAL="):
            return line.split("=", 1)[1].strip()
    return None


def ensure_build_dir_matches_generator(build_dir: Path, generator: str) -> None:
    existing = get_cmake_cache_generator(build_dir)
    if existing and existing != generator:
        print(
            f"Removing existing build directory because it was configured with a different generator: {existing}"
        )
        shutil.rmtree(build_dir)


def run_command(command: list[str], **kwargs: object) -> None:
    print(f"> {' '.join(command)}")
    subprocess.run(command, cwd=ROOT, check=True, **kwargs)


def copy_windows_mingw_runtime_dependencies(output_dir: Path) -> None:
    if platform.system() != "Windows":
        return

    gcc_path = shutil.which("gcc") or shutil.which("g++")
    if not gcc_path:
        return

    toolchain_bin = Path(gcc_path).resolve().parent
    runtime_names = [
        "libgcc_s_seh-1.dll",
        "libstdc++-6.dll",
        "libwinpthread-1.dll",
    ]
    output_dir.mkdir(parents=True, exist_ok=True)
    for runtime_name in runtime_names:
        source = toolchain_bin / runtime_name
        if source.is_file() is False:
            continue
        shutil.copy2(source, output_dir / runtime_name)


def main() -> int:
    parser = argparse.ArgumentParser(description="Build and package FreeCAD CommandTab native backend for the current host platform.")
    parser.add_argument("--native-platform", type=normalize_native_platform, default=None,
                        help="Target platform: linux, macos, windows. Defaults to current host platform.")
    parser.add_argument("--qt-major", default=os.environ.get("QT_MAJOR", DEFAULT_QT_MAJOR),
                        choices=["5", "6"], help="Qt major version to build.")
    parser.add_argument("--build-dir", type=Path,
                        help="CMake build directory. Defaults to build/native-<platform>-qt<major>.")
    parser.add_argument("--output-dir", type=Path,
                        help="Native binary output directory. Defaults to freecad_commandtab/native/bin/<platform>/qt<major>.")
    parser.add_argument("--package-dir", type=Path, default=DEFAULT_PACKAGE_DIR,
                        help="Packaged addon output directory.")
    parser.add_argument("--cmake-prefix-path", help="Additional CMAKE_PREFIX_PATH for Qt discovery.")
    parser.add_argument("--qt-install-dir", help="Path to the Qt installation root for CMake discovery.")
    parser.add_argument("--qt-rcc-executable", help="Override Qt rcc executable path.")
    parser.add_argument("--cmake-executable", help="CMake executable path or command. Defaults to CMAKE env var or cmake.")
    parser.add_argument("--generator", help="CMake generator to use. Defaults to Ninja when available, otherwise selects a Windows Visual Studio/NMake generator if possible.")
    parser.add_argument("--build-config", default=os.environ.get("CMAKE_BUILD_TYPE", DEFAULT_BUILD_CONFIG),
                        help=f"CMake build configuration for multi-config generators. Defaults to {DEFAULT_BUILD_CONFIG}.")
    args = parser.parse_args()

    native_platform = args.native_platform or detect_native_platform()
    qt_major = args.qt_major
    cmake_executable = resolve_executable(args.cmake_executable or os.environ.get("CMAKE", DEFAULT_CMAKE_EXECUTABLE))
    build_dir = args.build_dir or ROOT / f"build/native-{native_platform}-qt{qt_major}"
    output_dir = args.output_dir or ROOT / "freecad_commandtab" / "native" / "bin" / native_platform / f"qt{qt_major}"
    package_dir = args.package_dir

    print(f"Building for platform: {native_platform}")
    print(f"Qt major: {qt_major}")
    print(f"Build directory: {build_dir}")
    print(f"Native output directory: {output_dir}")
    print(f"Package directory: {package_dir}")
    print(f"CMake executable: {cmake_executable}")
    print(f"Build configuration: {args.build_config}")

    cmake_args = [
        cmake_executable,
        "-S",
        str(ROOT / "freecad_commandtab" / "native" / "cpp"),
        "-B",
        str(build_dir),
        f"-DFREECAD_COMMANDTAB_QT_MAJOR_VERSION={qt_major}",
        f"-DFREECAD_COMMANDTAB_NATIVE_OUTPUT_DIR={output_dir}",
        f"-DCMAKE_BUILD_TYPE={args.build_config}",
    ]
    qt_install_dir: Path | None = None
    if args.qt_install_dir:
        qt_install_dir = Path(args.qt_install_dir)
        if not qt_install_dir.exists():
            raise RuntimeError(f"Qt install directory not found: {qt_install_dir}")
    else:
        for env_var in (f"QT{qt_major}_DIR", "QT_INSTALL_DIR", "QT_DIR"):
            env_value = os.environ.get(env_var)
            if env_value:
                candidate = Path(env_value)
                if candidate.exists():
                    qt_install_dir = candidate
                    break
                print(f"Warning: {env_var} is set but points to a missing path: {candidate}")
        if qt_install_dir is None:
            qt_install_dir = find_qt_install_dir(qt_major)
            if qt_install_dir:
                print(f"Auto-detected Qt install directory: {qt_install_dir}")

    if args.cmake_prefix_path:
        cmake_args.append(f"-DCMAKE_PREFIX_PATH={args.cmake_prefix_path}")
    if qt_install_dir:
        print(f"Qt install directory: {qt_install_dir}")

    qt_toolchain = infer_qt_toolchain(qt_install_dir)
    if qt_toolchain:
        print(f"Detected Qt toolchain: {qt_toolchain}")

    chosen_generator: str | None = None
    if args.generator:
        cmake_args.extend(["-G", args.generator])
        chosen_generator = args.generator
    else:
        fallback = choose_default_generator(native_platform, cmake_executable, qt_toolchain)
        if fallback:
            cmake_args.extend(fallback)
            chosen_generator = fallback[1]
        elif native_platform == "windows":
            raise RuntimeError(
                "No supported Windows build generator found. Install Ninja or a Visual Studio / NMake build toolchain, or pass --generator explicitly."
            )

    if chosen_generator is not None:
        ensure_build_dir_matches_generator(build_dir, chosen_generator)

    env: dict[str, str] | None = None
    if chosen_generator and "NMake" in chosen_generator:
        env = get_visual_studio_env()
        if env is None:
            raise RuntimeError(
                "Unable to initialize a Visual Studio build environment for NMake. "
                "Install Visual Studio C++ build tools or run the helper from a developer command prompt."
            )

    if chosen_generator and "NMake" in chosen_generator:
        qt_install_dir = prepare_windows_msvc_qt_sdk(qt_install_dir, env=env)
        qt_toolchain = infer_qt_toolchain(qt_install_dir) or "msvc"
        if qt_install_dir:
            print(f"Prepared Qt install directory: {qt_install_dir}")

    if qt_install_dir:
        cmake_args.append(f"-DCMAKE_PREFIX_PATH={qt_install_dir}")
    if args.qt_rcc_executable:
        cmake_args.append(f"-DFREECAD_COMMANDTAB_QT_RCC_EXECUTABLE={args.qt_rcc_executable}")

    run_command(cmake_args, env=env or None)
    run_command(
        [
            cmake_executable,
            "--build",
            str(build_dir),
            "--config",
            str(args.build_config),
            "--parallel",
        ],
        env=env or None,
    )
    if native_platform == "windows" and qt_toolchain == "mingw":
        copy_windows_mingw_runtime_dependencies(output_dir)

    library_path = output_dir / ("freecad_commandtab_native_backend.dll" if native_platform == "windows" else
                                 "libfreecad_commandtab_native_backend.dylib" if native_platform == "macos" else
                                 "libfreecad_commandtab_native_backend.so")
    maybe_relink_macos_qt_dependencies(library_path)

    run_command([sys.executable, str(ROOT / "tools" / "verify_native_matrix.py"),
                 "--root", str(ROOT / "freecad_commandtab" / "native" / "bin"),
                 "--platform", native_platform, "--qt", f"qt{qt_major}"])
    run_command([sys.executable, str(ROOT / "tools" / "package_addon.py"),
                 "--output", str(package_dir)])
    run_command([sys.executable, str(ROOT / "tools" / "verify_native_matrix.py"),
                 "--root", str(package_dir / "freecad_commandtab" / "native" / "bin"),
                 "--platform", native_platform, "--qt", f"qt{qt_major}"])

    print(f"Done: {package_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
