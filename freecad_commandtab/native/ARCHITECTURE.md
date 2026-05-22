# Native CommandTab Architecture

## Goal

Deliver a native C++/Qt CommandTab runtime, with Python limited to unavoidable FreeCAD addon integration points.

## Current Boundary

### Native C++ owns

- CommandTab shell/widgets
- Workbench/page/panel rendering
- Native settings dialog and customization dialog
- Structure import/export/reset operations
- Runtime view refresh and model updates

### Python owns

- FreeCAD addon entrypoint (`InitGui.py`)
- startup orchestration (`freecad_commandtab/native/bootstrap.py`)
- bridge/load of native shared library (`freecad_commandtab/native/bridge.py`)
- command dispatch into FreeCAD GUI APIs
- metadata extraction from FreeCAD command/workbench APIs
- macOS/French translation fallback for command metadata when FreeCAD.app does
  not expose localized QAction text early enough

## Native-only Policy

Legacy Python ribbon UI modules were removed from this repository.

- no `freecad_commandtab/core/*`
- no `freecad_commandtab/dialogs/*`
- no root legacy wrappers (`FCBinding.py`, `CommandTab.py`, ...)
- no Python UI fallback at startup

If native backend loading fails, CommandTab is not attached and diagnostics are written.

## Startup Flow

1. FreeCAD loads `InitGui.py`
1. `bootstrap.activate_native_if_requested()` requests native controller activation
1. `bootstrap.attach_ui()` attaches native shell and applies post-startup visibility/theme hooks
1. bridge/controller keeps runtime state and handles UI-triggered actions

## Build/Packaging Contract

- Native backend target: `freecad_commandtab_native_backend`
- Build definition: `freecad_commandtab/native/cpp/CMakeLists.txt`
- Runtime binary location:
  - `freecad_commandtab/native/bin/linux/qt5|qt6`
  - `freecad_commandtab/native/bin/windows/qt5|qt6`
  - `freecad_commandtab/native/bin/macos/qt5|qt6`
- Packaging script excludes removed legacy Python UI paths by design.
- Packaging script also excludes local development environments such as `.qt`
  and `.venv-qt68`.

## Platform Isolation

- Linux and macOS native binaries live in separate runtime folders.
- macOS-specific Qt relinking is applied to the macOS `.dylib` only.
- The macOS/French TS translation fallback is guarded by `platform.system() ==
  "Darwin"` and a French locale check.
- Linux should continue to use FreeCAD's normal Qt translation and metadata APIs.

## Handover Rules

- Do not add new UI/runtime behavior to `InitGui.py`.
- Keep `bootstrap.py` deterministic and startup-focused.
- Add runtime features in C++ first; extend Python bridge only when required for FreeCAD API access.
- Keep user state in FreeCAD user paths, never inside installed addon source tree.
