#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

BUILD_DIR="${BUILD_DIR:-}"
QT_MAJOR="${QT_MAJOR:-6}"
PACKAGE_DIR="${PACKAGE_DIR:-dist/CommandTab}"

detect_native_platform() {
  case "$(uname -s)" in
    Linux*) echo "linux" ;;
    Darwin*) echo "macos" ;;
    MINGW*|MSYS*|CYGWIN*) echo "windows" ;;
    *) echo "unknown" ;;
  esac
}

normalize_native_platform() {
  case "$1" in
    darwin|Darwin|mac|Mac|macos|MacOS|osx|OSX) echo "macos" ;;
    linux|Linux) echo "linux" ;;
    windows|Windows|win|Win) echo "windows" ;;
    *) echo "$1" ;;
  esac
}

relink_macos_qt_dependencies() {
  local library_path="$1"
  local freecad_lib_dir="${FREECAD_LIB_DIR:-/Applications/FreeCAD.app/Contents/Resources/lib}"
  if [[ "$NATIVE_PLATFORM" != "macos" || ! -f "$library_path" || ! -d "$freecad_lib_dir" ]]; then
    return 0
  fi
  if ! command -v install_name_tool >/dev/null 2>&1; then
    return 0
  fi

  local changes=(
    "/opt/homebrew/opt/qtsvg/lib/QtSvg.framework/Versions/A/QtSvg:$freecad_lib_dir/libQt6Svg.6.dylib"
    "/opt/homebrew/opt/qtbase/lib/QtWidgets.framework/Versions/A/QtWidgets:$freecad_lib_dir/libQt6Widgets.6.dylib"
    "/opt/homebrew/opt/qtbase/lib/QtGui.framework/Versions/A/QtGui:$freecad_lib_dir/libQt6Gui.6.dylib"
    "/opt/homebrew/opt/qtbase/lib/QtCore.framework/Versions/A/QtCore:$freecad_lib_dir/libQt6Core.6.dylib"
    "/usr/local/opt/qtsvg/lib/QtSvg.framework/Versions/A/QtSvg:$freecad_lib_dir/libQt6Svg.6.dylib"
    "/usr/local/opt/qtbase/lib/QtWidgets.framework/Versions/A/QtWidgets:$freecad_lib_dir/libQt6Widgets.6.dylib"
    "/usr/local/opt/qtbase/lib/QtGui.framework/Versions/A/QtGui:$freecad_lib_dir/libQt6Gui.6.dylib"
    "/usr/local/opt/qtbase/lib/QtCore.framework/Versions/A/QtCore:$freecad_lib_dir/libQt6Core.6.dylib"
    "@rpath/QtSvg.framework/Versions/A/QtSvg:$freecad_lib_dir/libQt6Svg.6.dylib"
    "@rpath/QtWidgets.framework/Versions/A/QtWidgets:$freecad_lib_dir/libQt6Widgets.6.dylib"
    "@rpath/QtGui.framework/Versions/A/QtGui:$freecad_lib_dir/libQt6Gui.6.dylib"
    "@rpath/QtCore.framework/Versions/A/QtCore:$freecad_lib_dir/libQt6Core.6.dylib"
  )

  local change
  for change in "${changes[@]}"; do
    local old="${change%%:*}"
    local new="${change#*:}"
    if otool -L "$library_path" | grep -Fq "$old" && [[ -f "$new" ]]; then
      install_name_tool -change "$old" "$new" "$library_path"
    fi
  done
}

NATIVE_PLATFORM="${NATIVE_PLATFORM:-$(detect_native_platform)}"
NATIVE_PLATFORM="$(normalize_native_platform "$NATIVE_PLATFORM")"
if [[ "$NATIVE_PLATFORM" == "unknown" ]]; then
  echo "Unsupported host platform. Set NATIVE_PLATFORM=linux, macos, or windows." >&2
  exit 2
fi

if [[ -z "$BUILD_DIR" ]]; then
  BUILD_DIR="build/native-${NATIVE_PLATFORM}-qt${QT_MAJOR}"
fi
OUTPUT_DIR="${OUTPUT_DIR:-$ROOT_DIR/freecad_commandtab/native/bin/${NATIVE_PLATFORM}/qt${QT_MAJOR}}"
PYTHON="${PYTHON:-python3}"
if ! command -v "$PYTHON" >/dev/null 2>&1; then
  PYTHON="python"
fi

CMAKE="${CMAKE:-cmake}"
if ! command -v "$CMAKE" >/dev/null 2>&1; then
  echo "CMake executable not found: $CMAKE" >&2
  echo "Install CMake or set CMAKE to the path of the CMake executable." >&2
  exit 1
fi

echo "[1/5] Configure native backend (Qt${QT_MAJOR})"
configure_args=(
  -S freecad_commandtab/native/cpp
  -B "$BUILD_DIR"
  -DFREECAD_COMMANDTAB_QT_MAJOR_VERSION="${QT_MAJOR}"
  -DFREECAD_COMMANDTAB_NATIVE_OUTPUT_DIR="${OUTPUT_DIR}"
)
if [[ -n "${FREECAD_COMMANDTAB_QT_RCC_EXECUTABLE:-}" ]]; then
  configure_args+=(-DFREECAD_COMMANDTAB_QT_RCC_EXECUTABLE="$FREECAD_COMMANDTAB_QT_RCC_EXECUTABLE")
fi
if [[ -n "${CMAKE_PREFIX_PATH:-}" ]]; then
  configure_args+=(-DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH")
fi
if command -v ninja >/dev/null 2>&1; then
  configure_args+=(-G Ninja)
fi
"$CMAKE" "${configure_args[@]}"

echo "[2/5] Build native backend"
cmake --build "$BUILD_DIR" --parallel
relink_macos_qt_dependencies "$OUTPUT_DIR/libfreecad_commandtab_native_backend.dylib"

echo "[3/5] Verify current native matrix (advisory)"
"$PYTHON" tools/verify_native_matrix.py --root freecad_commandtab/native/bin \
  --platform "$NATIVE_PLATFORM" --qt "qt${QT_MAJOR}"

echo "[4/5] Build addon package folder"
"$PYTHON" tools/package_addon.py --output "$PACKAGE_DIR"

echo "[5/5] Verify packaged native matrix (advisory)"
"$PYTHON" tools/verify_native_matrix.py --root "$PACKAGE_DIR/freecad_commandtab/native/bin" \
  --platform "$NATIVE_PLATFORM" --qt "qt${QT_MAJOR}"

echo "Done: $PACKAGE_DIR"
