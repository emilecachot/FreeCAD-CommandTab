# FreeCAD CommandTab Native

This folder contains the native CommandTab runtime.

## Scope

- Native Qt/C++ commandtab shell
- Native workbench/page updates
- Native structure handling and runtime dialogs
- Native backend loaded as shared library (`ctypes` bridge)

Python remains only for:

- FreeCAD addon entrypoint (`InitGui.py`)
- startup orchestration
- command dispatch integration with FreeCAD

## Build

From repository root:

```bash
cmake -S freecad_commandtab/native/cpp -B build/native \
  -DFREECAD_COMMANDTAB_QT_MAJOR_VERSION=6 \
  -DFREECAD_COMMANDTAB_NATIVE_OUTPUT_DIR="$(pwd)/freecad_commandtab/native/bin/linux/qt6"

cmake --build build/native --config Release --parallel
```

For macOS, keep the same CMake target and place the `.dylib` under the macOS runtime folder:

```bash
cmake -S freecad_commandtab/native/cpp -B build/native-macos-qt6 \
  -DFREECAD_COMMANDTAB_QT_MAJOR_VERSION=6 \
  -DFREECAD_COMMANDTAB_NATIVE_OUTPUT_DIR="$(pwd)/freecad_commandtab/native/bin/macos/qt6"

cmake --build build/native-macos-qt6 --config Release --parallel
```

For Qt5, replace `6` with `5` and output to `.../qt5`.
The helper `tools/build_and_package_local.sh` detects Linux/macOS automatically and writes to `freecad_commandtab/native/bin/<platform>/qt<major>/`.

For the tested FreeCAD.app Qt 6.8.3 runtime on macOS:

```bash
BUILD_DIR=build/native-macos-qt683-names \
CMAKE_PREFIX_PATH="$PWD/.qt/6.8.3/macos" \
FREECAD_COMMANDTAB_QT_RCC_EXECUTABLE=/opt/homebrew/Cellar/qtbase/6.11.1/share/qt/libexec/rcc \
./tools/build_and_package_local.sh
```

After building, verify the `.dylib` links to FreeCAD.app Qt and not Homebrew Qt:

```bash
otool -L freecad_commandtab/native/bin/macos/qt6/libfreecad_commandtab_native_backend.dylib
```

## Runtime Activation

- Addon includes native mode support only.
- Enable by marker file `FREECAD_COMMANDTAB_NATIVE` in addon root or env `FREECAD_COMMANDTAB_NATIVE=1`.

Expected backend location:

- `freecad_commandtab/native/bin/<platform>/qt<major>/`
- `<platform>` = `linux`, `windows`, `macos`

## macOS Translation Notes

FreeCAD.app on macOS can expose incomplete or non-localized QAction text for some
workbenches during early metadata export. The bridge contains a macOS/French-only
fallback that reads bundled FreeCAD `.ts` files and resolves command labels from
their Qt contexts, for example `CmdSketcherCreateLine` and
`CmdTechDrawPageDefault`.

This fallback is guarded by the host platform and locale, so Linux keeps using
the normal FreeCAD/Qt translation path.

## Related Docs

- Architecture and migration notes: `freecad_commandtab/native/ARCHITECTURE.md`
- Packaging: `tools/package_addon.py`
- Matrix check: `tools/verify_native_matrix.py`
