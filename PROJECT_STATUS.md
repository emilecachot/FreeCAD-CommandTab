# Project Status

Last updated: 2026-05-21

## Current Goal

Keep the existing Linux native CommandTab runtime working while adding and stabilizing the macOS runtime for FreeCAD.app.

## Current State

- Linux support is retained through `freecad_commandtab/native/bin/linux/qt<major>/`.
- macOS support is added through `freecad_commandtab/native/bin/macos/qt6/`.
- The local macOS package installs and starts in FreeCAD.app with the native ribbon enabled.
- The installed macOS native backend is linked against FreeCAD.app Qt 6.8.3 libraries, not Homebrew Qt.
- The distributable package excludes local development folders such as `.qt` and `.venv-qt68`.

## macOS Work Completed

- Added macOS platform detection and packaging paths.
- Added macOS Qt 6 native backend build support.
- Added support for using an explicit `CMAKE_PREFIX_PATH` during local native builds.
- Added support for an explicit Qt `rcc` executable when the Qt SDK does not expose it in the expected location.
- Relinked the macOS backend to FreeCAD.app bundled Qt dylibs.
- Added macOS startup and metadata-cache fixes for workbench icons and command icons.
- Added a macOS/French translation fallback using bundled FreeCAD `.ts` files.
- Added command-label fallback for contexts such as `CmdTechDrawPageDefault` and `CmdSketcherCreateLine`.
- Added Sketcher-specific command context aliases where FreeCAD command IDs do not map mechanically to Qt translation contexts.

## Linux Compatibility Notes

- The macOS translation fallback is guarded by `platform.system() == "Darwin"` and a French locale check.
- Linux continues to use the normal FreeCAD/Qt translation path.
- The Linux binary path and matrix verification layout were not replaced.
- Shared changes that affect all platforms are limited to packaging hygiene, metadata cache versioning, and trying an available FreeCAD translation before falling back to a humanized command ID.

## Verified Locally

- Python syntax check for `freecad_commandtab/native/bridge.py`.
- macOS package size remains about 73 MB after excluding local Qt and virtualenv folders.
- Installed macOS backend links to:
  - `/Applications/FreeCAD.app/Contents/Resources/lib/libQt6Core.6.dylib`
  - `/Applications/FreeCAD.app/Contents/Resources/lib/libQt6Gui.6.dylib`
  - `/Applications/FreeCAD.app/Contents/Resources/lib/libQt6Widgets.6.dylib`
  - `/Applications/FreeCAD.app/Contents/Resources/lib/libQt6Svg.6.dylib`
- Sketcher command translation coverage was checked against the bundled French TS data for all Sketcher commands present in the current metadata cache.

## Local macOS Build Command

```bash
BUILD_DIR=build/native-macos-qt683-names \
CMAKE_PREFIX_PATH="$PWD/.qt/6.8.3/macos" \
FREECAD_COMMANDTAB_QT_RCC_EXECUTABLE=/opt/homebrew/Cellar/qtbase/6.11.1/share/qt/libexec/rcc \
./tools/build_and_package_local.sh
```

## Local macOS Install Path

```text
~/Library/Application Support/FreeCAD/v1-1/Mod/FreeCAD-CommandTab
```

## Cache To Clear During macOS Testing

```text
~/Library/Application Support/FreeCAD/v1-1/Cache/FreeCAD-CommandTab/CommandTabNativeMetadata.json
~/Library/Application Support/FreeCAD/v1-1/Cache/FreeCAD-CommandTab/qt_action_cache.json
~/Library/Application Support/FreeCAD/v1-1/Cache/FreeCAD-CommandTab/icons
~/Library/Application Support/FreeCAD/v1-1/Cache/FreeCAD-CommandTab/qt-action-icons
```

## Remaining Checks Before Release

- Run the addon once on Linux and confirm the existing Linux native backend still loads.
- Run the native matrix verifier against the final packaged addon.
- Rebuild the GitHub Actions multi-OS artifact and confirm Linux, Windows, and macOS folders are present.
