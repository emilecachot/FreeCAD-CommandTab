from __future__ import annotations

import json
import logging
import os
import platform
import shutil
import tempfile
import time
import xml.etree.ElementTree as ET
from contextlib import contextmanager

import FreeCAD as App
import FreeCADGui as Gui
from PySide.QtCore import QEvent, QLocale, QObject, QPoint, Qt, QTimer, QTranslator, qVersion
from PySide.QtGui import QIcon
from PySide.QtWidgets import QApplication, QDockWidget, QMenu, QToolBar, QWidget

import Parameters_CommandTab
from freecad_commandtab import paths

try:
    from freecad_commandtab import startup_trace as StartupTrace
except Exception:
    class _StartupTraceFallback:
        @staticmethod
        def reset_session(_session_name: str = "startup"):
            return ""

        @staticmethod
        def mark(_label: str, **_fields):
            return None

        @staticmethod
        @contextmanager
        def span(_label: str, **_fields):
            yield

    StartupTrace = _StartupTraceFallback()
from freecad_commandtab.native import runtime_diagnostics as NativeRuntimeDiagnostics

_logger = logging.getLogger(__name__)

try:
    import shiboken6 as _shiboken
except Exception:
    try:
        import shiboken2 as _shiboken
    except Exception:
        _shiboken = None


translate = App.Qt.translate

PRODUCTIVITY_MODE_FLAG = paths.addon_path("FREECAD_COMMANDTAB_PRODUCTIVITY")
NATIVE_RUNTIME_STATUS_FILE = paths.user_cache_path("FREECAD_COMMANDTAB_NATIVE_STATUS.json")
NATIVE_RUNTIME_STATUS_TEMP_FILE = os.path.join(
    tempfile.gettempdir(), "freecad-commandtab-native-status.json"
)
EMBEDDED_THEME_ROOT = paths.addon_path(
    "Resources", "stylesheets", "theme"
)
LEGACY_THEME_ROOT = paths.addon_path("Theme-main")
THEME_ROOT = (
    EMBEDDED_THEME_ROOT
    if os.path.isdir(EMBEDDED_THEME_ROOT)
    else LEGACY_THEME_ROOT
)
OPEN_PREFERENCES_CFG = os.path.join(
    THEME_ROOT, "OpenPreferences", "OpenPreferences.cfg"
)
ONDSEL_DEFAULTS_CFG = paths.addon_path("Ondsel-defaults.cfg")
THEME_SANITIZED_CACHE_ROOT = os.path.join(
    tempfile.gettempdir(), "freecad-commandtab-theme-qss"
)
THEME_APPLIED_KEY = "ThemeBootstrapVersion"
ONDSEL_DEFAULTS_APPLIED_KEY = "OndselDefaultsBootstrapVersion"
THEME_MODE_KEY = "ThemeBootstrapMode"
THEME_VERSION = "Theme-bootstrap-20260403"
ONDSEL_DEFAULTS_VERSION = "Ondsel-defaults-bootstrap-20260413"
MAIN_WINDOW_PREFERENCES = App.ParamGet("User parameter:BaseApp/Preferences/MainWindow")
COMMANDTAB_PREFERENCES = App.ParamGet("User parameter:BaseApp/Preferences/Mod/FreeCAD-CommandTab")
BUNDLED_COMMANDTAB_ICON_THEME_NAME = "FreeCAD-CommandTab-Modern"
_OPEN_COMMANDTAB_DIALOGS = []
_LEGACY_FALLBACK_ACTIVE = False
_LEGACY_FALLBACK_ERROR = ""
_NATIVE_WORKBENCH_UI_HOOK_INSTALLED = False
_NATIVE_WORKBENCH_UI_TIMER = None
_NATIVE_WORKBENCH_UI_EVENT_FILTER = None
_NATIVE_BRAND_MENU_FILTER = None
_NATIVE_MENU_BAR_FILTER = None
_NATIVE_SHUTDOWN_HOOK_INSTALLED = False
_NATIVE_WORKBENCH_UI_HIDE_LAST_TS = 0.0
_STRICT_NATIVE_ENV_KEY = "FREECAD_COMMANDTAB_STRICT_NATIVE"
_ALLOW_LEGACY_FALLBACK_ENV_KEY = "FREECAD_COMMANDTAB_ALLOW_LEGACY_FALLBACK"
_ENABLE_PY_EVENT_FILTERS_ENV_KEY = "FREECAD_COMMANDTAB_ENABLE_PY_EVENT_FILTERS"
_VISIBILITY_DEBUG_FILE = paths.user_cache_path("FREECAD_COMMANDTAB_VISIBILITY_DEBUG.log")
_COMMANDTAB_TRANSLATORS = []


class MainWindowNotReadyError(RuntimeError):
    """Raised when FreeCAD main window is not yet ready for UI attachment."""


def _qt_object_is_valid(obj) -> bool:
    if obj is None:
        return False
    if _shiboken is None:
        return True
    try:
        return bool(_shiboken.isValid(obj))
    except Exception:
        return False


def _qt_object_name(obj) -> str:
    if _qt_object_is_valid(obj) is False:
        return ""
    try:
        return str(obj.objectName() or "").strip()
    except Exception:
        return ""


def _qt_event_type(event):
    if event is None:
        return None
    try:
        return event.type()
    except Exception:
        return None


def _qt_is_destroy_event(event_type) -> bool:
    if event_type is None:
        return True
    destroy_events = []
    for event_name in ["Destroy", "DeferredDelete", "ChildRemoved"]:
        event_value = getattr(QEvent.Type, event_name, None)
        if event_value is not None:
            destroy_events.append(event_value)
    return event_type in destroy_events


def _python_ui_event_filters_enabled() -> bool:
    env_value = str(os.environ.get(_ENABLE_PY_EVENT_FILTERS_ENV_KEY, "") or "").strip().lower()
    if env_value in ["1", "true", "yes", "on"]:
        return True
    if env_value in ["0", "false", "no", "off"]:
        return False

    # Default to safe mode on Linux Qt6 runtimes: some hosts deliver delete
    # events to Python event filters while C++ widgets are already tearing down,
    # which can crash in wrapped QObject access (PySide6/objectName).
    return not (platform.system().lower() == "linux" and _qt_major_version_local() >= 6)


def _native_ui_hide_min_interval_ms() -> int:
    raw_value = str(os.environ.get("FREECAD_COMMANDTAB_UI_HIDE_MIN_INTERVAL_MS", "110") or "110").strip()
    try:
        return max(30, int(raw_value))
    except Exception:
        return 110


def _qt_major_version_local() -> int:
    try:
        version_text = str(qVersion() or "")
    except Exception:
        return 0
    major_text, _, _rest = version_text.partition(".")
    try:
        return int(major_text)
    except Exception:
        return 0


def _native_bridge_module():
    from freecad_commandtab.native import bridge as NativeCommandTabBridge

    return NativeCommandTabBridge


def _append_visibility_debug(message: str) -> None:
    try:
        os.makedirs(os.path.dirname(_VISIBILITY_DEBUG_FILE), exist_ok=True)
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        with open(_VISIBILITY_DEBUG_FILE, "a", encoding="utf-8") as handle:
            handle.write(f"[{timestamp}] {str(message).strip()}\n")
    except Exception:
        pass


def reset_visibility_debug_log() -> None:
    try:
        os.makedirs(os.path.dirname(_VISIBILITY_DEBUG_FILE), exist_ok=True)
        with open(_VISIBILITY_DEBUG_FILE, "w", encoding="utf-8") as handle:
            handle.write("")
    except Exception:
        pass


def is_main_window_not_ready_error(error: Exception) -> bool:
    return isinstance(error, MainWindowNotReadyError)


def _require_main_window():
    try:
        main_window = Gui.getMainWindow()
    except Exception as error:
        raise MainWindowNotReadyError(str(error)) from error
    if _qt_object_is_valid(main_window) is False:
        raise MainWindowNotReadyError("Gui.getMainWindow() returned no valid main window")
    return main_window


def _load_fc_binding_module():
    raise RuntimeError("Legacy Python UI is disabled (native-only mode)")


def _connect_workbench_activated_once(mw, callback) -> None:
    return


def _activate_legacy_commandtab_immediately(binding_module) -> None:
    return


def _refresh_legacy_main_window_bindings(main_window, binding_module=None) -> None:
    return


def _legacy_commandtab_dock_exists() -> bool:
    return False


def _schedule_legacy_commandtab_activation_retry(
    binding_module,
    retry_count: int = 12,
    retry_delay_ms: int = 450,
) -> None:
    return


def _install_native_shutdown_hook() -> None:
    global _NATIVE_SHUTDOWN_HOOK_INSTALLED

    if _NATIVE_SHUTDOWN_HOOK_INSTALLED is True:
        return
    application = QApplication.instance()
    if application is None:
        return

    def _shutdown_native_runtime():
        try:
            _native_bridge_module().shutdown_native_commandtab()
        except Exception:
            pass

    try:
        application.aboutToQuit.connect(_shutdown_native_runtime)
        _NATIVE_SHUTDOWN_HOOK_INSTALLED = True
    except Exception:
        pass


def _write_native_runtime_status(active: bool, error: str = ""):
    try:
        payload = dict(_native_bridge_module().native_runtime_status())
        payload["active"] = bool(active)
        payload["error"] = str(error or "")
        payload["fallbackUsed"] = bool(_LEGACY_FALLBACK_ACTIVE)
        payload["legacyFallbackAvailable"] = _legacy_fallback_available()
        payload["legacyFallbackError"] = str(_LEGACY_FALLBACK_ERROR or "")
        payload["strictNativeMode"] = _strict_native_mode_enabled()
        payload["legacyFallbackAllowed"] = _legacy_fallback_allowed()
        for status_path in [NATIVE_RUNTIME_STATUS_FILE, NATIVE_RUNTIME_STATUS_TEMP_FILE]:
            try:
                os.makedirs(os.path.dirname(status_path), exist_ok=True)
                with open(status_path, "w", encoding="utf-8") as handle:
                    json.dump(payload, handle, ensure_ascii=True, indent=2)
            except Exception:
                pass
    except Exception:
        pass


def _legacy_fallback_available() -> bool:
    return False


def _strict_native_mode_enabled() -> bool:
    value = os.environ.get(_STRICT_NATIVE_ENV_KEY, "")
    return NativeRuntimeDiagnostics.bool_env(value, False)


def _legacy_fallback_allowed() -> bool:
    # Native-only policy: legacy Python UI fallback is explicitly disabled.
    return False


def _set_legacy_fallback_state(active: bool, error: str = "") -> None:
    global _LEGACY_FALLBACK_ACTIVE
    global _LEGACY_FALLBACK_ERROR

    _LEGACY_FALLBACK_ACTIVE = bool(active)
    _LEGACY_FALLBACK_ERROR = str(error or "")


def is_legacy_fallback_active() -> bool:
    return bool(_LEGACY_FALLBACK_ACTIVE)


def _emit_native_startup_diagnostics(
    *,
    native_requested: bool,
    error: str,
    fallback_used: bool,
) -> None:
    try:
        status = dict(_native_bridge_module().native_runtime_status())
    except Exception:
        status = {}
    summary = NativeRuntimeDiagnostics.summarize_native_startup(
        status=status,
        error=error,
        native_requested=native_requested,
        strict_native_mode=_strict_native_mode_enabled(),
        legacy_fallback_allowed=_legacy_fallback_allowed(),
        legacy_fallback_available=_legacy_fallback_available(),
        legacy_fallback_used=fallback_used,
    )
    for line in NativeRuntimeDiagnostics.format_console_lines(summary):
        try:
            App.Console.PrintWarning(f"{line}\n")
        except Exception:
            print(line)


def _parameter_has_bool(group, setting_name: str) -> bool:
    try:
        for setting in group.GetContents():
            if setting[0] == "Boolean" and setting[1] == setting_name:
                return True
    except Exception:
        pass
    return False


def _commandtab_bool_setting(setting_name: str, default: bool) -> bool:
    if _parameter_has_bool(COMMANDTAB_PREFERENCES, setting_name):
        try:
            return bool(COMMANDTAB_PREFERENCES.GetBool(setting_name))
        except Exception:
            return bool(default)
    try:
        COMMANDTAB_PREFERENCES.SetBool(setting_name, bool(default))
        App.saveParameter()
    except Exception:
        pass
    return bool(default)


def configure_logging() -> None:
    with StartupTrace.span("bootstrap.configure_logging"):
        logging.getLogger("urllib3").setLevel(logging.INFO)


def _theme_mode() -> str:
    mode = COMMANDTAB_PREFERENCES.GetString("NativeThemeMode").strip().lower()
    if mode in ["dark", "light"]:
        return mode

    stylesheet_name = MAIN_WINDOW_PREFERENCES.GetString("StyleSheet").strip().lower()
    if "openlight" in stylesheet_name:
        return "light"
    if "opendark" in stylesheet_name:
        return "dark"

    main_window_theme = MAIN_WINDOW_PREFERENCES.GetString("Theme").strip().lower()
    if "light" in main_window_theme:
        return "light"
    if "dark" in main_window_theme:
        return "dark"

    try:
        palette = Gui.getMainWindow().palette()
        probes = [palette.window().color(), palette.base().color(), palette.button().color()]
        valid_probes = [probe for probe in probes if probe.isValid()]
        if valid_probes:
            average = sum(probe.lightnessF() for probe in valid_probes) / float(len(valid_probes))
            return "dark" if average < 0.49 else "light"
        color = palette.window().color()
        return "dark" if color.lightness() < 128 else "light"
    except Exception:
        return "dark"


def _use_modern_icon_theme() -> bool:
    return False


def _sanitized_theme_stylesheet_path(stylesheet_path: str) -> str:
    if os.path.isfile(stylesheet_path) is False:
        return stylesheet_path

    try:
        relative_parent = os.path.relpath(os.path.dirname(stylesheet_path), THEME_ROOT)
    except Exception:
        relative_parent = os.path.basename(os.path.dirname(stylesheet_path))

    target_dir = os.path.join(THEME_SANITIZED_CACHE_ROOT, relative_parent)
    os.makedirs(target_dir, exist_ok=True)
    target_path = os.path.join(target_dir, os.path.basename(stylesheet_path))

    try:
        source_mtime = os.path.getmtime(stylesheet_path)
        if os.path.isfile(target_path) and os.path.getmtime(target_path) >= source_mtime:
            return target_path
    except Exception:
        pass

    try:
        with open(stylesheet_path, encoding="utf-8") as handle:
            payload = handle.read()
    except Exception:
        return stylesheet_path

    sanitized_payload = payload.replace("qss:", "")
    if sanitized_payload == payload:
        return stylesheet_path

    try:
        with open(target_path, "w", encoding="utf-8") as handle:
            handle.write(sanitized_payload)
        return target_path
    except Exception:
        return stylesheet_path


def _selected_theme_assets() -> tuple[str, str]:
    mode = _theme_mode()
    if mode == "light":
        return (
            _sanitized_theme_stylesheet_path(
                os.path.join(THEME_ROOT, "OpenLight", "OpenLight.qss")
            ),
            _sanitized_theme_stylesheet_path(
                os.path.join(
                    THEME_ROOT, "OpenLight", "overlay", "OpenLight_Overlay.qss"
                )
            ),
        )
    return (
        _sanitized_theme_stylesheet_path(
            os.path.join(THEME_ROOT, "OpenDark", "OpenDark.qss")
        ),
        _sanitized_theme_stylesheet_path(
            os.path.join(THEME_ROOT, "OpenDark", "overlay", "OpenDark_Overlay.qss")
        ),
    )


def _parameter_group_from_parts(parts: list[str]):
    path = "User parameter:"
    if parts:
        path = f"{path}{'/'.join(parts)}"
    return App.ParamGet(path)


def _apply_open_preferences_group(node, path_parts: list[str], should_apply_leaf=None) -> None:
    name = str(node.attrib.get("Name", "")).strip()
    if node.tag == "FCParamGroup":
        next_parts = list(path_parts)
        if name and name != "Root":
            next_parts.append(name)
        for child in list(node):
            _apply_open_preferences_group(child, next_parts, should_apply_leaf)
        return

    if not name:
        return

    if callable(should_apply_leaf):
        try:
            if should_apply_leaf(path_parts, node.tag, name, node.attrib.get("Value"), node.text) is False:
                return
        except Exception:
            return

    value = node.attrib.get("Value")
    group = _parameter_group_from_parts(path_parts)

    if node.tag == "FCBool":
        group.SetBool(name, str(value).strip().lower() in ["1", "true", "yes", "on"])
        return
    if node.tag == "FCInt":
        try:
            group.SetInt(name, int(str(value).strip() or "0"))
        except Exception:
            pass
        return
    if node.tag == "FCUInt":
        try:
            group.SetUnsigned(name, int(str(value).strip() or "0"))
        except Exception:
            pass
        return
    if node.tag == "FCFloat":
        try:
            group.SetFloat(name, float(str(value).strip() or "0"))
        except Exception:
            pass
        return
    if node.tag == "FCText":
        text_value = value if value is not None else (node.text or "")
        group.SetString(name, str(text_value))


def _apply_preferences_cfg_once(
    cfg_path: str,
    applied_key: str,
    applied_version: str,
    *,
    should_apply_leaf=None,
) -> None:
    if os.path.isfile(cfg_path) is False:
        return

    applied_marker = COMMANDTAB_PREFERENCES.GetString(applied_key).strip()
    if applied_marker == applied_version:
        return

    try:
        root = ET.parse(cfg_path).getroot()
    except Exception:
        return

    for child in list(root):
        _apply_open_preferences_group(child, [], should_apply_leaf)

    COMMANDTAB_PREFERENCES.SetString(applied_key, applied_version)
    App.saveParameter()


def _ondsel_defaults_should_apply(path_parts: list[str], node_tag: str, name: str, _value, _text) -> bool:
    normalized_path = tuple(str(part or "").strip() for part in path_parts)

    if normalized_path == ("BaseApp", "Preferences", "General"):
        return name in {"QtStyle"}

    if normalized_path == ("BaseApp", "Preferences", "View"):
        return name in {
            "EditSketcherFontSize",
            "ShowRotationCenter",
            "ShowNaviCube",
            "ShowSelectionBoundingBox",
            "UseAutoRotation",
            "UseVBO",
            "AntiAliasing",
            "RenderCache",
            "SegmentsPerGeometry",
            "BoundingBoxFontSize",
            "ViewScalingFactor",
            "EnableBacklight",
            "Gradient",
            "RadialGradient",
            "RandomColor",
            "Simple",
            "UseBackgroundColorMid",
            "BacklightIntensity",
            "DefaultShapeTransparency",
        }

    if normalized_path == ("BaseApp", "Preferences", "Mod", "Draft"):
        return name in {"gridTransparency"}

    if normalized_path == ("BaseApp", "Preferences", "Mod", "Sketcher"):
        return name in {
            "AutoRemoveRedundants",
            "HideUnits",
            "UseSystemDecimals",
            "ShowDimensionalName",
        }

    if normalized_path == ("BaseApp", "Preferences", "Mod", "Sketcher", "General"):
        return name in {
            "BSplineCombVisible",
            "NotifyConstraintSubstitutions",
            "ShowGrid",
            "GridAuto",
            "RestoreCamera",
            "ForceOrtho",
            "SectionView",
            "GridDivLinePattern",
            "GridLinePattern",
        }

    if normalized_path == ("BaseApp", "Preferences", "Mod", "Sketcher", "Constraints"):
        return name in {"UnifiedCoincident", "AutoHorVer"}

    if normalized_path == ("BaseApp", "Preferences", "Mod", "Sketcher", "Snap"):
        return name in {"SnapToGrid"}

    if normalized_path == ("BaseApp", "Preferences", "Mod", "Sketcher", "View"):
        return name in {
            "EdgeWidth",
            "ConstructionWidth",
            "InternalWidth",
            "ExternalWidth",
            "EdgePattern",
            "ConstructionPattern",
            "InternalPattern",
            "ExternalPattern",
        }

    return False


def _apply_open_preferences_once() -> None:
    with StartupTrace.span("bootstrap.apply_open_preferences_once"):
        _apply_preferences_cfg_once(
            OPEN_PREFERENCES_CFG,
            THEME_APPLIED_KEY,
            THEME_VERSION,
        )


def _apply_ondsel_defaults_once() -> None:
    with StartupTrace.span("bootstrap.apply_ondsel_defaults_once"):
        _apply_preferences_cfg_once(
            ONDSEL_DEFAULTS_CFG,
            ONDSEL_DEFAULTS_APPLIED_KEY,
            ONDSEL_DEFAULTS_VERSION,
            should_apply_leaf=_ondsel_defaults_should_apply,
        )


def _looks_like_theme_stylesheet(stylesheet_path: str) -> bool:
    normalized = str(stylesheet_path or "").strip().replace("\\", "/").lower()
    if normalized == "":
        return False
    if "theme-main" in normalized:
        return True
    return normalized.endswith("/opendark.qss") or normalized.endswith("/openlight.qss") or normalized.endswith("/opendark_overlay.qss") or normalized.endswith("/openlight_overlay.qss") or normalized in {
        "opendark.qss",
        "openlight.qss",
        "opendark_overlay.qss",
        "openlight_overlay.qss",
    }


def _preserve_external_stylesheet() -> bool:
    current_stylesheet = MAIN_WINDOW_PREFERENCES.GetString("StyleSheet").strip()
    if current_stylesheet != "" and _looks_like_theme_stylesheet(current_stylesheet) is False:
        return True

    current_overlay = MAIN_WINDOW_PREFERENCES.GetString("OverlayActiveStyleSheet").strip()
    if current_overlay != "" and _looks_like_theme_stylesheet(current_overlay) is False:
        return True

    current_commandtab_stylesheet = Parameters_CommandTab.Settings.GetStringSetting("Stylesheet")
    if str(current_commandtab_stylesheet or "").strip() != "" and _looks_like_theme_stylesheet(str(current_commandtab_stylesheet)) is False:
        return True

    return False


def _ensure_sketcher_display_defaults() -> None:
    sketcher_general = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Sketcher/General")
    sketcher_snap = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Sketcher/Snap")
    keep_grid_visible = _commandtab_bool_setting("ShowSketcherGrid", True)
    keep_snap_enabled = _commandtab_bool_setting("SnapSketcherGrid", True)

    show_grid_was_enabled = sketcher_general.GetBool("ShowGrid")
    snap_to_grid_was_enabled = sketcher_snap.GetBool("SnapToGrid")

    parameters_changed = False

    if keep_grid_visible is True:
        if show_grid_was_enabled is not True:
            parameters_changed = True
        sketcher_general.SetBool("ShowGrid", True)
        if sketcher_general.GetBool("GridAuto") is not True:
            parameters_changed = True
        sketcher_general.SetBool("GridAuto", True)
    if keep_snap_enabled is True:
        if snap_to_grid_was_enabled is not True:
            parameters_changed = True
        sketcher_snap.SetBool("SnapToGrid", True)

    try:
        active_workbench = Gui.activeWorkbench()
        active_workbench_name = active_workbench.name()
    except Exception:
        active_workbench_name = ""

    if active_workbench_name == "SketcherWorkbench":
        if keep_grid_visible is True and show_grid_was_enabled is False:
            try:
                Gui.runCommand("Sketcher_Grid", 0)
            except Exception:
                pass
        if keep_snap_enabled is True and snap_to_grid_was_enabled is False:
            try:
                Gui.runCommand("Sketcher_Snap", 0)
            except Exception:
                pass

    if parameters_changed is True:
        App.saveParameter()


def _reload_stylesheets() -> None:
    with StartupTrace.span("bootstrap.reload_stylesheets"):
        try:
            Gui.runCommand("Std_ReloadStyleSheet", 0)
            return
        except Exception:
            pass

        try:
            window = Gui.getMainWindow()
            window.style().unpolish(window)
            window.style().polish(window)
            window.update()
        except Exception:
            pass


def _apply_native_commandtab_theme_stylesheet() -> None:
    try:
        _native_bridge_module().apply_native_commandtab_theme_stylesheet()
    except Exception:
        pass


def apply_theme_preferences() -> None:
    with StartupTrace.span("bootstrap.apply_theme_preferences"):
        if Parameters_CommandTab.MODERN_COMMANDTAB_STYLE_ENABLED is not True:
            return
        _apply_ondsel_defaults_once()
        _ensure_sketcher_display_defaults()
        if os.path.isdir(THEME_ROOT) is False:
            return
        if _preserve_external_stylesheet() is True:
            applied_mode = _theme_mode()
            if COMMANDTAB_PREFERENCES.GetString(THEME_MODE_KEY).strip().lower() != applied_mode:
                COMMANDTAB_PREFERENCES.SetString(THEME_MODE_KEY, applied_mode)
                App.saveParameter()
            return

        _apply_open_preferences_once()

        stylesheet, overlay_stylesheet = _selected_theme_assets()
        if os.path.isfile(stylesheet) is False:
            return

        stylesheet_changed = False
        if MAIN_WINDOW_PREFERENCES.GetString("StyleSheet") != stylesheet:
            MAIN_WINDOW_PREFERENCES.SetString("StyleSheet", stylesheet)
            stylesheet_changed = True
        if os.path.isfile(overlay_stylesheet) and (
            MAIN_WINDOW_PREFERENCES.GetString("OverlayActiveStyleSheet")
            != overlay_stylesheet
        ):
            MAIN_WINDOW_PREFERENCES.SetString(
                "OverlayActiveStyleSheet", overlay_stylesheet
            )
            stylesheet_changed = True

        current_commandtab_stylesheet = Parameters_CommandTab.Settings.GetStringSetting("Stylesheet")
        if current_commandtab_stylesheet != stylesheet:
            Parameters_CommandTab.Settings.SetStringSetting("Stylesheet", stylesheet)
            Parameters_CommandTab.STYLESHEET = stylesheet
            stylesheet_changed = True
        else:
            Parameters_CommandTab.STYLESHEET = stylesheet

        applied_mode = _theme_mode()
        if COMMANDTAB_PREFERENCES.GetString(THEME_MODE_KEY).strip().lower() != applied_mode:
            COMMANDTAB_PREFERENCES.SetString(THEME_MODE_KEY, applied_mode)
            stylesheet_changed = True

        App.saveParameter()
        if stylesheet_changed is True:
            _reload_stylesheets()


def activate_native_if_requested() -> bool:
    with StartupTrace.span("bootstrap.activate_native_if_requested"):
        NativeCommandTabBridge = _native_bridge_module()
        native_mode_requested = (
            NativeCommandTabBridge.is_native_mode_requested() is True
            or bool(getattr(Parameters_CommandTab, "PREFER_NATIVE_COMMANDTAB", False)) is True
        )

        if native_mode_requested is False:
            _set_legacy_fallback_state(False, "")
            _write_native_runtime_status(False, "")
            return False

        try:
            active = NativeCommandTabBridge.activate_native_commandtab(
                force=bool(getattr(Parameters_CommandTab, "PREFER_NATIVE_COMMANDTAB", False))
            )
            if active is True:
                _set_legacy_fallback_state(False, "")
            _write_native_runtime_status(active, "")
            if active is True:
                print(
                    translate(
                        "FreeCAD CommandTab", "Activating native Qt C++ CommandTab UI..."
                    )
                )
            return active
        except Exception as e:
            _set_legacy_fallback_state(False, str(e))
            _write_native_runtime_status(False, str(e))
            if bool(getattr(Parameters_CommandTab, "DEBUG_MODE", False)) is True:
                print(f"Native commandtab activation failed: {e}")
            return False


def _bundled_commandtab_icon_theme_source_path() -> str:
    return paths.addon_path(
        "Export", "IconThemes", BUNDLED_COMMANDTAB_ICON_THEME_NAME
    )


def _bundled_commandtab_icon_theme_install_root() -> str:
    return os.path.join(App.getUserAppDataDir(), "Gui", "Icons")


def _bundled_commandtab_icon_theme_install_path() -> str:
    return os.path.join(
        _bundled_commandtab_icon_theme_install_root(), BUNDLED_COMMANDTAB_ICON_THEME_NAME
    )


def _ensure_bundled_commandtab_icon_theme_installed() -> bool:
    with StartupTrace.span("bootstrap.ensure_bundled_icon_theme_installed"):
        source_path = _bundled_commandtab_icon_theme_source_path()
        target_path = _bundled_commandtab_icon_theme_install_path()

        if os.path.isdir(source_path) is False:
            return False

        os.makedirs(_bundled_commandtab_icon_theme_install_root(), exist_ok=True)
        source_index = os.path.join(source_path, "index.theme")
        target_index = os.path.join(target_path, "index.theme")
        should_copy = os.path.isdir(target_path) is False
        if should_copy is False and os.path.exists(source_index) and os.path.exists(target_index):
            should_copy = os.path.getmtime(source_index) > os.path.getmtime(target_index)

        if should_copy is True:
            if os.path.isdir(target_path):
                shutil.rmtree(target_path, ignore_errors=True)
            shutil.copytree(source_path, target_path)
        return os.path.isdir(target_path)


def _activate_bundled_icon_theme_lightweight() -> bool:
    with StartupTrace.span("bootstrap.activate_bundled_icon_theme_lightweight"):
        if _use_modern_icon_theme() is False:
            return False
        if _ensure_bundled_commandtab_icon_theme_installed() is False:
            return False

        search_paths = list(QIcon.themeSearchPaths())
        for candidate in [
            _bundled_commandtab_icon_theme_install_root(),
            paths.addon_path("Export", "IconThemes"),
        ]:
            if candidate not in search_paths and os.path.isdir(candidate):
                search_paths.append(candidate)

        QIcon.setThemeSearchPaths(search_paths)
        QIcon.setThemeName(BUNDLED_COMMANDTAB_ICON_THEME_NAME)

        theme_preferences = App.ParamGet("User parameter:BaseApp/Preferences/Bitmaps/Theme")
        theme_preferences.SetString("SearchPath", _bundled_commandtab_icon_theme_install_root())
        theme_preferences.SetString("Name", BUNDLED_COMMANDTAB_ICON_THEME_NAME)
        theme_preferences.SetBool("UseIconTheme", True)
        theme_preferences.SetBool("ThemeSearchPaths", True)
        return True


def _deactivate_bundled_icon_theme_lightweight() -> None:
    with StartupTrace.span("bootstrap.deactivate_bundled_icon_theme_lightweight"):
        try:
            QIcon.setThemeName("")
        except Exception:
            pass
        try:
            filtered_paths = []
            for candidate in QIcon.themeSearchPaths():
                if "FreeCAD-CommandTab-Modern" in str(candidate):
                    continue
                if str(candidate).endswith(os.path.join("Export", "IconThemes")):
                    continue
                filtered_paths.append(candidate)
            QIcon.setThemeSearchPaths(filtered_paths)
        except Exception:
            pass

        theme_preferences = App.ParamGet("User parameter:BaseApp/Preferences/Bitmaps/Theme")
        if theme_preferences.GetString("Name").strip() == BUNDLED_COMMANDTAB_ICON_THEME_NAME:
            theme_preferences.SetString("Name", "")
        search_path = theme_preferences.GetString("SearchPath").strip()
        if "FreeCAD-CommandTab-Modern" in search_path or search_path == _bundled_commandtab_icon_theme_install_root():
            theme_preferences.SetString("SearchPath", "")
        theme_preferences.SetBool("UseIconTheme", False)
        theme_preferences.SetBool("ThemeSearchPaths", False)
        App.saveParameter()


def maybe_activate_bundled_icon_theme() -> None:
    with StartupTrace.span("bootstrap.maybe_activate_bundled_icon_theme"):
        _deactivate_bundled_icon_theme_lightweight()


def ensure_structure_files(current_structure_version: int) -> None:
    with StartupTrace.span(
        "bootstrap.ensure_structure_files",
        currentStructureVersion=int(current_structure_version),
    ):
        configured_structure = str(
            getattr(Parameters_CommandTab, "COMMANDTAB_STRUCTURE_JSON", "") or ""
        ).strip()
        file = configured_structure or paths.user_state_path("CommandTabStructure.json")
        file_default = os.path.join(
            os.path.dirname(file), "CommandTabStructure_default.json"
        )
        source = paths.addon_path("CreateStructure.txt")
        source_default = paths.addon_path("CreateStructure.txt")

        new_default_needed = True
        commandtab_structure_version = Parameters_CommandTab.Settings.GetIntSetting(
            "CommandTabStructureVersion"
        )
        if commandtab_structure_version >= current_structure_version:
            new_default_needed = False

        if os.path.isfile(file) is False:
            os.makedirs(os.path.dirname(file), exist_ok=True)
            shutil.copy(source, file)

        if os.path.isfile(file_default) is False or new_default_needed is True:
            os.makedirs(os.path.dirname(file_default), exist_ok=True)
            shutil.copy(source_default, file_default)
            Parameters_CommandTab.Settings.SetIntSetting(
                "CommandTabStructureVersion", current_structure_version
            )


def remove_test_workbench() -> None:
    with StartupTrace.span("bootstrap.remove_test_workbench"):
        try:
            Gui.removeWorkbench("TestWorkbench")
        except Exception:
            pass


def configure_overlay_preferences() -> None:
    with StartupTrace.span("bootstrap.configure_overlay_preferences"):
        use_custom_overlay_flag = paths.addon_path("OVERLAY_DISABLED")
        if (
            Parameters_CommandTab.USE_FC_OVERLAY is False
            or os.path.exists(use_custom_overlay_flag) is True
        ):
            preferences = App.ParamGet("User parameter:BaseApp/Preferences/DockWindows")
            preferences.SetBool("ActivateOverlay", False)

            preferences = App.ParamGet(
                "User parameter:BaseApp/MainWindow/DockWindows/OverlayTop"
            )
            preferences.SetString("Widgets", "")

        if Parameters_CommandTab.USE_FC_OVERLAY is True:
            preferences = App.ParamGet("User parameter:BaseApp/Preferences/DockWindows")
            preferences.SetBool("ActivateOverlay", True)


def attach_ui(native_commandtab_active: bool) -> None:
    with StartupTrace.span(
        "bootstrap.attach_ui", nativeCommandTabActive=bool(native_commandtab_active)
    ):
        print(translate("FreeCAD CommandTab", "Activating CommandTab UI..."))
        _require_main_window()
        NativeCommandTabBridge = _native_bridge_module()
        native_mode_requested = True
        native_ui_hide_active = False
        try:
            native_mode_requested = (
                NativeCommandTabBridge.is_native_mode_requested() is True
                or bool(getattr(Parameters_CommandTab, "PREFER_NATIVE_COMMANDTAB", False)) is True
            )
        except Exception:
            native_mode_requested = True

        if native_commandtab_active is False:
            try:
                native_error = str(NativeCommandTabBridge.native_runtime_status().get("lastError", "")).strip()
            except Exception:
                native_error = ""
            failure_error = native_error or translate(
                "FreeCAD CommandTab",
                "Native CommandTab activation failed and Python fallback is disabled",
            )
            _set_legacy_fallback_state(False, failure_error)
            _write_native_runtime_status(False, failure_error)
            _emit_native_startup_diagnostics(
                native_requested=bool(native_mode_requested),
                error=failure_error,
                fallback_used=False,
            )
            try:
                App.Console.PrintError(
                    "FreeCAD CommandTab native-only startup failed:\n"
                    + failure_error
                    + "\n"
                )
            except Exception:
                pass
            return

        _set_legacy_fallback_state(False, "")
        _write_native_runtime_status(True, "")
        print(translate("FreeCAD CommandTab", "CommandTab UI: native Qt C++ bridge active"))
        _install_native_shutdown_hook()
        _ensure_sketcher_display_defaults()
        _ensure_native_brand_menu_ready_deferred()
        native_startup_orchestration = False
        try:
            native_startup_orchestration = (
                NativeCommandTabBridge.is_cpp_startup_orchestration_enabled() is True
            )
        except Exception:
            native_startup_orchestration = False
        if native_startup_orchestration is True:
            try:
                native_ui_hide_active = (
                    NativeCommandTabBridge.enable_native_ui_hide_controller(True) is True
                )
            except Exception:
                native_ui_hide_active = False
        if native_ui_hide_active is False:
            _ensure_native_workbench_ui_hidden()
            QTimer.singleShot(0, _apply_native_commandtab_theme_stylesheet)
            QTimer.singleShot(90, _apply_native_commandtab_theme_stylesheet)


def _active_native_shell():
    try:
        return Gui.getMainWindow().findChild(QWidget, "FreeCADCommandTabNativeShell")
    except Exception:
        return None


def _active_legacy_commandtab_dock():
    return None


def _show_widget_safely(widget) -> bool:
    if _qt_object_is_valid(widget) is False:
        return False
    try:
        if isinstance(widget, QDockWidget):
            try:
                if int(widget.maximumHeight()) <= 0:
                    widget.setMaximumHeight(16777215)
            except Exception:
                pass
            try:
                if int(widget.minimumHeight()) <= 0:
                    widget.setMinimumHeight(48)
            except Exception:
                pass
        widget.setVisible(True)
    except Exception:
        pass
    try:
        widget.show()
    except Exception:
        pass
    try:
        widget.raise_()
    except Exception:
        pass
    try:
        return bool(widget.isVisible())
    except Exception:
        return True


def ensure_any_commandtab_visible() -> bool:
    _append_visibility_debug("ensure_any_commandtab_visible: begin")
    native_shell = _active_native_shell()
    if _qt_object_is_valid(native_shell):
        if _show_widget_safely(native_shell):
            _append_visibility_debug("ensure_any_commandtab_visible: native shell visible")
            return True
    _append_visibility_debug("ensure_any_commandtab_visible: native shell unavailable")
    return False


def ensure_commandtab_visible_deferred(retry_count: int = 10, delay_ms: int = 150) -> None:
    if QTimer is None:
        ensure_any_commandtab_visible()
        return

    attempts = {"remaining": max(1, int(retry_count))}
    interval_ms = max(40, int(delay_ms))
    _append_visibility_debug(
        "ensure_commandtab_visible_deferred: retry_count="
        + str(attempts["remaining"])
        + " delay_ms="
        + str(interval_ms)
    )

    def _attempt():
        visible = ensure_any_commandtab_visible() is True
        _append_visibility_debug(
            "ensure_commandtab_visible_deferred: attempt visible=" + str(bool(visible))
        )
        if visible:
            return
        attempts["remaining"] -= 1
        if attempts["remaining"] <= 0:
            _append_visibility_debug("ensure_commandtab_visible_deferred: exhausted retries")
            return
        QTimer.singleShot(interval_ms, _attempt)

    QTimer.singleShot(0, _attempt)


def _active_python_commandtab():
    return None


def _widget_class_name(widget: QWidget | None) -> str:
    if widget is None:
        return ""
    try:
        meta_object = widget.metaObject()
        if meta_object is None:
            return ""
        class_name = meta_object.className()
        if isinstance(class_name, bytes):
            return class_name.decode("utf-8", errors="ignore")
        return str(class_name or "")
    except Exception:
        return ""


def _exec_menu(menu: QMenu, global_pos: QPoint) -> None:
    try:
        menu.exec(global_pos)
        return
    except Exception:
        pass
    try:
        menu.exec_(global_pos)
    except Exception:
        pass


def _build_native_brand_menu(anchor: QWidget) -> QMenu | None:
    try:
        mw = Gui.getMainWindow()
    except Exception:
        return None
    if mw is None:
        return None

    menu_bar = mw.menuBar()
    if menu_bar is None:
        return None

    menu = QMenu(anchor)
    menu.setObjectName("CommandTabNativeBrandMenu")
    menu.setToolTipsVisible(True)

    action_added = False
    for action in menu_bar.actions():
        if action is None:
            continue
        text = str(action.text() or "").replace("&", "").strip()
        if text == "":
            continue
        if action.isSeparator():
            if action_added:
                menu.addSeparator()
            continue
        menu.addAction(action)
        action_added = True

    if action_added is False:
        menu.deleteLater()
        return None
    return menu


def _native_brand_menu_has_actions() -> bool:
    try:
        mw = Gui.getMainWindow()
    except Exception:
        return False
    if mw is None:
        return False

    try:
        menu_bar = mw.menuBar()
    except Exception:
        menu_bar = None
    if menu_bar is None:
        return False

    for action in menu_bar.actions():
        if action is None:
            continue
        try:
            text = str(action.text() or "").replace("&", "").strip()
        except Exception:
            text = ""
        if text == "" or action.isSeparator():
            continue
        return True
    return False


class _NativeMenuBarEventFilter(QObject):
    def eventFilter(self, watched, event):
        if _qt_object_is_valid(watched) is False:
            return False

        event_type = _qt_event_type(event)
        if _qt_is_destroy_event(event_type):
            return False

        watched_events = {
            QEvent.Type.LayoutRequest,
            QEvent.Type.Show,
            QEvent.Type.ChildAdded,
        }
        action_added = getattr(QEvent.Type, "ActionAdded", None)
        if action_added is not None:
            watched_events.add(action_added)
        action_removed = getattr(QEvent.Type, "ActionRemoved", None)
        if action_removed is not None:
            watched_events.add(action_removed)

        if event_type in watched_events:
            if QTimer is not None:
                # Re-evaluate logo menu readiness when FreeCAD menus are rebuilt.
                QTimer.singleShot(0, _hide_redundant_native_workbench_ui)
            return False

        return False


def _ensure_native_menu_bar_hook() -> bool:
    global _NATIVE_MENU_BAR_FILTER

    try:
        mw = Gui.getMainWindow()
    except Exception:
        return False
    if _qt_object_is_valid(mw) is False:
        return False

    try:
        menu_bar = mw.menuBar()
    except Exception:
        menu_bar = None
    if _qt_object_is_valid(menu_bar) is False:
        return False

    if bool(menu_bar.property("commandtabMenuBarHook")) is True:
        return True

    _NATIVE_MENU_BAR_FILTER = _NativeMenuBarEventFilter(menu_bar)
    try:
        menu_bar.installEventFilter(_NATIVE_MENU_BAR_FILTER)
    except Exception:
        _NATIVE_MENU_BAR_FILTER = None
        return False

    menu_bar.setProperty("commandtabMenuBarHook", True)
    return True


def _ensure_native_brand_menu_ready_deferred(retry_count: int = 80, delay_ms: int = 150) -> None:
    if QTimer is None:
        return

    attempts = {"remaining": max(1, int(retry_count))}
    interval_ms = max(60, int(delay_ms))

    def _attempt():
        # Install the menu-bar hook as early as possible; action population can
        # happen before the native shell is fully discoverable.
        _ensure_native_menu_bar_hook()

        native_shell = _active_native_shell()
        if _qt_object_is_valid(native_shell) is False:
            attempts["remaining"] -= 1
            if attempts["remaining"] > 0:
                QTimer.singleShot(interval_ms, _attempt)
            return

        hook_ready = _ensure_native_brand_menu_hook(native_shell)
        menu_ready = hook_ready and _native_brand_menu_has_actions()
        try:
            mw = Gui.getMainWindow()
            if mw is not None and platform.system().lower() != "darwin":
                hide_menu_bar = bool(
                    getattr(Parameters_CommandTab, "HIDE_MENUBAR_IN_NATIVE_MODE", True)
                )
                mw.menuBar().setVisible(bool(not hide_menu_bar or menu_ready is False))
        except Exception:
            pass

        if menu_ready is True:
            return

        attempts["remaining"] -= 1
        if attempts["remaining"] > 0:
            QTimer.singleShot(interval_ms, _attempt)

    QTimer.singleShot(0, _attempt)


class _NativeBrandMenuEventFilter(QObject):
    def eventFilter(self, watched, event):
        if _qt_object_is_valid(watched) is False:
            return False

        event_type = _qt_event_type(event)
        if _qt_is_destroy_event(event_type):
            return False

        if event_type in [QEvent.Type.MouseButtonRelease, QEvent.Type.MouseButtonDblClick]:
            menu = _build_native_brand_menu(watched)
            if menu is None:
                return False
            try:
                anchor = watched.mapToGlobal(QPoint(0, watched.height()))
            except Exception:
                anchor = None
            if anchor is None:
                menu.deleteLater()
                return False
            _exec_menu(menu, anchor)
            menu.deleteLater()
            return True

        if event_type == QEvent.Type.KeyPress:
            try:
                key = int(event.key())
            except Exception:
                key = -1
            # Enter, Return, Space
            if key in [16777220, 16777221, 32]:
                menu = _build_native_brand_menu(watched)
                if menu is None:
                    return False
                try:
                    anchor = watched.mapToGlobal(QPoint(0, watched.height()))
                except Exception:
                    anchor = None
                if anchor is None:
                    menu.deleteLater()
                    return False
                _exec_menu(menu, anchor)
                menu.deleteLater()
                return True

        return False


def _is_redundant_native_workbench_widget(widget: QWidget | None, native_shell: QWidget | None) -> bool:
    if _qt_object_is_valid(widget) is False or _qt_object_is_valid(native_shell) is False:
        return False
    if widget is native_shell:
        return False
    try:
        is_descendant = native_shell.isAncestorOf(widget)
    except Exception:
        is_descendant = False
    if is_descendant:
        return False

    try:
        mw = Gui.getMainWindow()
    except Exception:
        mw = None
    if mw is None:
        return False

    if isinstance(widget, QToolBar):
        protected_toolbar_names = {"SearchBar"}
        protected_areas = {
            Qt.ToolBarArea.LeftToolBarArea,
            Qt.ToolBarArea.RightToolBarArea,
            Qt.ToolBarArea.BottomToolBarArea,
        }

        try:
            parent_widget = widget.parentWidget()
        except Exception:
            parent_widget = None
        parent_name = _qt_object_name(parent_widget)
        toolbar_name = _qt_object_name(widget)
        if parent_name in ["statusBar", "StatusBarArea"]:
            return False
        try:
            area = mw.toolBarArea(widget)
        except Exception:
            area = Qt.ToolBarArea.NoToolBarArea
        if area in protected_areas or toolbar_name in protected_toolbar_names:
            return False
        return True

    object_name = _qt_object_name(widget)
    class_name = _widget_class_name(widget)
    if object_name in ["WbTabBar", "WbTabBarMore"] or "WorkbenchComboBox" in class_name:
        return True
    return False


class _NativeWorkbenchUiEventFilter(QObject):
    def eventFilter(self, watched, event):
        if _qt_object_is_valid(watched) is False:
            return False

        native_shell = _active_native_shell()
        if _qt_object_is_valid(native_shell) is False:
            return False

        event_type = _qt_event_type(event)
        if _qt_is_destroy_event(event_type):
            return False
        if event_type == QEvent.Type.ChildAdded:
            try:
                child = event.child()
            except Exception:
                child = None
            if isinstance(child, QWidget) and _qt_object_is_valid(child):
                try:
                    child.installEventFilter(self)
                except Exception:
                    pass
                if _is_redundant_native_workbench_widget(child, native_shell):
                    try:
                        child.hide()
                    except Exception:
                        pass
                    QTimer.singleShot(90, _hide_redundant_native_workbench_ui)
            return False

        show_event_types = [
            QEvent.Type.Show,
            QEvent.Type.LayoutRequest,
            QEvent.Type.PolishRequest,
        ]
        show_to_parent = getattr(QEvent.Type, "ShowToParent", None)
        if show_to_parent is not None:
            show_event_types.append(show_to_parent)
        if event_type in show_event_types:
            if _is_redundant_native_workbench_widget(watched, native_shell):
                try:
                    watched.hide()
                except Exception:
                    pass
                QTimer.singleShot(90, _hide_redundant_native_workbench_ui)
        return False


def _ensure_native_brand_menu_hook(native_shell: QWidget) -> bool:
    global _NATIVE_BRAND_MENU_FILTER

    if native_shell is None:
        return False

    brand_widget = native_shell.findChild(QWidget, "CommandTabBrandBadge")
    if brand_widget is None:
        return False

    if bool(brand_widget.property("commandtabBrandMenuHook")) is True:
        return True

    _NATIVE_BRAND_MENU_FILTER = _NativeBrandMenuEventFilter(brand_widget)
    brand_widget.installEventFilter(_NATIVE_BRAND_MENU_FILTER)
    brand_widget.setProperty("commandtabBrandMenuHook", True)
    brand_widget.setFocusPolicy(Qt.FocusPolicy.StrongFocus)
    try:
        brand_widget.setCursor(Qt.CursorShape.PointingHandCursor)
    except Exception:
        try:
            brand_widget.setCursor(Qt.PointingHandCursor)
        except Exception:
            pass
    try:
        brand_widget.setToolTip(translate("FreeCAD CommandTab", "Menu"))
    except Exception:
        pass
    return True


def _hide_redundant_native_workbench_ui(*, force: bool = False) -> None:
    global _NATIVE_WORKBENCH_UI_HIDE_LAST_TS

    now = time.perf_counter()
    min_interval = max(0.03, float(_native_ui_hide_min_interval_ms()) / 1000.0)
    if force is False and _NATIVE_WORKBENCH_UI_HIDE_LAST_TS > 0.0:
        if (now - _NATIVE_WORKBENCH_UI_HIDE_LAST_TS) < min_interval:
            return
    _NATIVE_WORKBENCH_UI_HIDE_LAST_TS = now

    native_shell = _active_native_shell()
    if _qt_object_is_valid(native_shell) is False:
        return

    try:
        mw = Gui.getMainWindow()
    except Exception:
        return
    if _qt_object_is_valid(mw) is False:
        return

    _ensure_native_menu_bar_hook()
    brand_menu_hook_ready = _ensure_native_brand_menu_hook(native_shell)
    brand_menu_ready = brand_menu_hook_ready and _native_brand_menu_has_actions()
    if brand_menu_ready is False:
        try:
            QTimer.singleShot(180, _hide_redundant_native_workbench_ui)
        except Exception:
            pass
    if platform.system().lower() != "darwin":
        hide_menu_bar = bool(
            getattr(Parameters_CommandTab, "HIDE_MENUBAR_IN_NATIVE_MODE", True)
        )
        try:
            mw.menuBar().setVisible(bool(not hide_menu_bar or brand_menu_ready is False))
        except Exception:
            pass

    protected_toolbar_names = {"SearchBar"}
    protected_areas = {
        Qt.ToolBarArea.LeftToolBarArea,
        Qt.ToolBarArea.RightToolBarArea,
        Qt.ToolBarArea.BottomToolBarArea,
    }

    for toolbar in mw.findChildren(QToolBar):
        if _qt_object_is_valid(toolbar) is False or toolbar is native_shell:
            continue
        try:
            is_descendant = native_shell.isAncestorOf(toolbar)
        except Exception:
            is_descendant = False
        if is_descendant:
            continue

        try:
            parent_widget = toolbar.parentWidget()
        except Exception:
            parent_widget = None
        parent_name = _qt_object_name(parent_widget)
        toolbar_name = _qt_object_name(toolbar)

        if parent_name in ["statusBar", "StatusBarArea"]:
            toolbar.setEnabled(True)
            toolbar.setVisible(True)
            continue

        try:
            area = mw.toolBarArea(toolbar)
        except Exception:
            area = Qt.ToolBarArea.NoToolBarArea

        if area in protected_areas or toolbar_name in protected_toolbar_names:
            toolbar.setEnabled(True)
            toolbar.setVisible(True)
            continue

        toolbar.setVisible(False)

    for widget in mw.findChildren(QWidget):
        if _qt_object_is_valid(widget) is False or widget is native_shell:
            continue
        try:
            is_descendant = native_shell.isAncestorOf(widget)
        except Exception:
            is_descendant = False
        if is_descendant:
            continue

        object_name = _qt_object_name(widget)
        class_name = _widget_class_name(widget)

        if object_name in ["WbTabBar", "WbTabBarMore"] or "WorkbenchComboBox" in class_name:
            widget.setVisible(False)


def _ensure_native_workbench_ui_hidden() -> None:
    global _NATIVE_WORKBENCH_UI_HOOK_INSTALLED
    global _NATIVE_WORKBENCH_UI_TIMER
    global _NATIVE_WORKBENCH_UI_EVENT_FILTER

    _hide_redundant_native_workbench_ui(force=True)
    # Re-hide in short bursts to prevent transient workbench UI flashes while
    # FreeCAD finishes toolbar/tab construction during workbench switches.
    for delay_ms in [90, 240]:
        QTimer.singleShot(delay_ms, _hide_redundant_native_workbench_ui)

    if _python_ui_event_filters_enabled() is False:
        return

    if _NATIVE_WORKBENCH_UI_TIMER is None:
        try:
            mw = Gui.getMainWindow()
        except Exception:
            mw = None
        if mw is not None:
            timer = QTimer(mw)
            timer.setInterval(320)

            def _poll_redundant_native_workbench_ui():
                if _active_native_shell() is None:
                    try:
                        timer.stop()
                    except Exception:
                        pass
                    return
                _hide_redundant_native_workbench_ui()

            timer.timeout.connect(_poll_redundant_native_workbench_ui)
            timer.start()
            _NATIVE_WORKBENCH_UI_TIMER = timer

            if _NATIVE_WORKBENCH_UI_EVENT_FILTER is None:
                event_filter = _NativeWorkbenchUiEventFilter(mw)
                try:
                    mw.installEventFilter(event_filter)
                except Exception:
                    event_filter = None
                if event_filter is not None:
                    for widget in mw.findChildren(QWidget):
                        if widget is None:
                            continue
                        try:
                            widget.installEventFilter(event_filter)
                        except Exception:
                            pass
                    _NATIVE_WORKBENCH_UI_EVENT_FILTER = event_filter
    else:
        try:
            if _NATIVE_WORKBENCH_UI_TIMER.isActive() is False:
                _NATIVE_WORKBENCH_UI_TIMER.start()
        except Exception:
            pass

    if _NATIVE_WORKBENCH_UI_HOOK_INSTALLED is True:
        return

    try:
        mw = Gui.getMainWindow()
    except Exception:
        return
    if mw is None:
        return

    def _rehide_after_workbench_change(*_args):
        _ensure_sketcher_display_defaults()
        for delay_ms in [0, 120, 320]:
            QTimer.singleShot(delay_ms, _hide_redundant_native_workbench_ui)

    try:
        mw.workbenchActivated.connect(_rehide_after_workbench_change)
        _NATIVE_WORKBENCH_UI_HOOK_INSTALLED = True
    except Exception:
        pass


def set_commandtab_controls_enabled(enabled: bool) -> None:
    native_shell = _active_native_shell()
    if native_shell is not None:
        native_shell.setEnabled(bool(enabled))
        Gui.updateGui()
    return


def _register_dialog_instance(dialog) -> None:
    return


def _refresh_native_commandtab_after_dialog_change() -> None:
    return


def _show_dialog(dialog_module_name: str) -> object | None:
    return None


def show_design_dialog():
    try:
        native_bridge = _native_bridge_module()
        controller = getattr(native_bridge, "_ACTIVE_CONTROLLER", None)
        if controller is not None and controller.open_customization_dialog("", ""):
            return controller
        App.Console.PrintError(
            "FreeCAD CommandTab native-only mode: customization dialog unavailable.\n"
        )
    except Exception:
        return None
    return None


def show_settings_dialog():
    try:
        native_bridge = _native_bridge_module()
        controller = getattr(native_bridge, "_ACTIVE_CONTROLLER", None)
        if controller is not None and controller.open_settings_dialog(
            native_bridge._native_settings_state_json()
        ):
            return controller
        App.Console.PrintError(
            "FreeCAD CommandTab native-only mode: settings dialog unavailable.\n"
        )
    except Exception:
        return None
    return None


def _install_productivity_layer_deferred():
    debug_mode = bool(
        globals().get("Parameters_CommandTab", None)
        and getattr(Parameters_CommandTab, "DEBUG_MODE", False)
    )
    try:
        from freecad_commandtab import ergonomics as CommandTabErgonomics

        CommandTabErgonomics.install_productivity_layer()
    except Exception as e:
        if debug_mode is True:
            print(f"Productivity layer activation failed: {e}")


def _should_install_productivity_layer(native_commandtab_active: bool) -> bool:
    if native_commandtab_active is False:
        return True
    env_value = os.environ.get("FREECAD_COMMANDTAB_PRODUCTIVITY", "").strip().lower()
    if env_value in ["1", "true", "yes", "on"]:
        return True
    return os.path.exists(PRODUCTIVITY_MODE_FLAG)


def schedule_productivity_layer(native_commandtab_active: bool) -> None:
    with StartupTrace.span(
        "bootstrap.schedule_productivity_layer",
        nativeCommandTabActive=bool(native_commandtab_active),
    ):
        if _should_install_productivity_layer(native_commandtab_active) is True:
            delay_ms = 1600 if native_commandtab_active is True else 180
            StartupTrace.mark(
                "bootstrap.productivity_layer_scheduled",
                delayMs=int(delay_ms),
                nativeCommandTabActive=bool(native_commandtab_active),
            )
            QTimer.singleShot(delay_ms, _install_productivity_layer_deferred)


def _commandtab_locale_candidates() -> list[str]:
    raw_candidates: list[str] = []

    def _append_raw(value) -> None:
        value = str(value or "").strip()
        if value != "" and value not in raw_candidates:
            raw_candidates.append(value)

    try:
        _append_raw(Gui.getLocale())
    except Exception:
        pass

    try:
        _append_raw(App.ParamGet("User parameter:BaseApp/Preferences/General").GetString("Language"))
    except Exception:
        pass

    try:
        system_locale = QLocale.system()
        _append_raw(system_locale.name())
        _append_raw(QLocale.languageToString(system_locale.language()))
    except Exception:
        pass

    language_aliases = {
        "deutsch": "de",
        "english": "en",
        "francais": "fr",
        "français": "fr",
        "french": "fr",
        "german": "de",
        "polish": "pl",
        "polski": "pl",
        "spanish": "es",
        "espanol": "es",
        "español": "es",
    }

    candidates: list[str] = []

    def _append(value: str) -> None:
        value = str(value or "").strip()
        if value != "" and value not in candidates:
            candidates.append(value)

    for raw_value in raw_candidates:
        lowered = raw_value.lower().strip()
        normalized_name = (
            lowered.replace(" ", "")
            .replace("-", "_")
            .replace("(", "")
            .replace(")", "")
        )
        alias = language_aliases.get(normalized_name)
        if alias:
            _append(alias)

        compact = raw_value.replace("-", "_").strip()
        _append(compact)
        if "_" in compact:
            language, _, territory = compact.partition("_")
            if language and territory:
                _append(f"{language}-{territory.upper()}")
            _append(language)

    if any(candidate.lower().startswith("es_ar") or candidate.lower() == "es-ar" for candidate in candidates):
        _append("es-AR")
    if any(candidate.lower().startswith("es") for candidate in candidates):
        _append("es-ES")

    return candidates


def _install_commandtab_qm_translation() -> None:
    translations_dir = paths.addon_path("translations")
    if os.path.isdir(translations_dir) is False:
        return

    for locale_name in _commandtab_locale_candidates():
        if locale_name.strip().lower() in {"", "c", "en", "english"}:
            continue
        qm_name = f"FreeCAD_CommandTab_{locale_name}.qm"
        qm_path = os.path.join(translations_dir, qm_name)
        if os.path.isfile(qm_path) is False and "_" in locale_name:
            qm_path = os.path.join(
                translations_dir,
                f"FreeCAD_CommandTab_{locale_name.split('_', 1)[0]}.qm",
            )
        if os.path.isfile(qm_path) is False:
            continue

        translator = QTranslator(QApplication.instance())
        if translator.load(qm_path) is False:
            continue
        application = QApplication.instance()
        if application is None:
            return
        application.installTranslator(translator)
        _COMMANDTAB_TRANSLATORS.append(translator)
        return


def register_translations() -> None:
    with StartupTrace.span("bootstrap.register_translations"):
        add_language_path = getattr(Gui, "addLanguagePath", None)
        if callable(add_language_path):
            add_language_path(paths.addon_path("translations"))
        update_locale = getattr(Gui, "updateLocale", None)
        if callable(update_locale):
            update_locale()
        _install_commandtab_qm_translation()
