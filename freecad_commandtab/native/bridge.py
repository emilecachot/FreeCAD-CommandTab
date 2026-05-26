from __future__ import annotations

import base64
import copy
import ctypes
import hashlib
import json
import logging
import os
import platform
import re
import shutil
import time
import unicodedata
import xml.etree.ElementTree as ET
from contextlib import contextmanager
from pathlib import Path

import FreeCAD as App
import FreeCADGui as Gui
from PySide.QtCore import QByteArray, QFileInfo, QLocale, QCoreApplication, QSize, Qt, QTimer, qVersion
from PySide.QtGui import QApplication, QColor, QIcon, QPainter, QPixmap
from PySide.QtWidgets import QToolBar, QToolButton, QWidget

import Parameters_CommandTab
from freecad_commandtab import paths
from freecad_commandtab import theme as CommandTabTheme
from freecad_commandtab.native import command_metadata as NativeCommandMetadata

_logger = logging.getLogger(__name__)

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

NATIVE_MODE_FLAG = Path(paths.addon_path("FREECAD_COMMANDTAB_NATIVE"))
_ACTIVE_CONTROLLER = None
_NATIVE_RUNTIME_KIND = ""
_NATIVE_RUNTIME_PATH = ""
_NATIVE_RUNTIME_LIBRARY_PLATFORM = ""
_NATIVE_RUNTIME_LIBRARY_PLATFORM_SUPPORTED: bool | None = None
_NATIVE_RUNTIME_LAST_ERROR = ""
_NATIVE_RUNTIME_LAST_STAGE = "idle"
_WINDOWS_DLL_SEARCH_PATH_HANDLES: list[object] = []
_STRUCTURE_CACHE: dict[str, object] = {}
_COMMAND_INFO_CACHE: dict[str, dict] = {}
_COMMAND_ENTRY_CACHE: dict[tuple[object, ...], dict] = {}
_QT_ACTION_CACHE: dict[tuple[str, str], dict[str, dict]] = {}
_QT_ACTION_CACHE_FAILURES: dict[tuple[str, str], int] = {}
_WORKBENCH_TITLE_CACHE: dict[str, str] = {}
_WORKBENCH_ICON_CACHE: dict[str, str] = {}
_AVAILABLE_WORKBENCHES_CACHE: dict[str, object] = {}
_AVAILABLE_WORKBENCHES_CACHE_VALID = False
_AVAILABLE_WORKBENCHES_CACHE_TS = 0.0
_BASE_MODEL_CACHE: dict[str, object] = {}
_WORKBENCH_PANEL_CACHE: dict[tuple[tuple[str, str, int], str], list[dict]] = {}
_WORKBENCH_PAYLOAD_CACHE: dict[tuple[tuple[str, str, int], tuple[str, str], str, str], str] = {}
_MODEL_PAYLOAD_CACHE: dict[tuple[tuple[str, str, int], tuple[str, str], str, bool, str], str] = {}
_BOOTSTRAP_PAYLOAD_CACHE: dict[tuple[tuple[str, str, int], tuple[str, str], str, bool, str], str] = {}
_WORKBENCH_BOOTSTRAP_CACHE: dict[tuple[tuple[str, str, int], tuple[str, str], str, str], str] = {}
_WORKBENCH_BOOTSTRAP_STATE_CACHE: dict[str, dict[str, object]] = {}
_VARIANT_MENU_CACHE_MEMORY: dict[str, object] = {}
_STATIC_COMMAND_VARIANT_MENU_CACHE: dict[str, list[object]] | None = None
_DYNAMIC_WORKBENCH_COMMON_TOOLBARS = {
    "clipboard",
    "edit",
    "file",
    "help",
    "individual view",
    "individual views",
    "macro",
    "structure",
    "view",
    "workbench",
}
_COMMAND_DISPLAY_TEXT_OVERRIDES: dict[str, tuple[str, str]] = {
    "PartDesign_CompSketches": ("CmdPartDesignNewSketch", "New Sketch"),
    "PartDesign_NewSketch": ("CmdPartDesignNewSketch", "New Sketch"),
}
_WORKBENCH_PRELOAD_IN_PROGRESS = False
_WORKBENCH_PRELOAD_ENABLED = str(
    os.environ.get("FREECAD_COMMANDTAB_PRELOAD_WORKBENCHES", "0") or "0"
).strip().lower() in {"1", "true", "yes", "on"}
_CPP_BOOTSTRAP_PIPELINE_ENABLED = str(
    os.environ.get("FREECAD_COMMANDTAB_CPP_BOOTSTRAP_PIPELINE", "0") or "0"
).strip().lower() not in {"0", "false", "no", "off"}
_CPP_METADATA_PIPELINE_ENABLED = str(
    os.environ.get("FREECAD_COMMANDTAB_CPP_METADATA_PIPELINE", "1") or "1"
).strip().lower() not in {"0", "false", "no", "off"}
_CPP_UI_HIDE_ENABLED = str(
    os.environ.get("FREECAD_COMMANDTAB_CPP_UI_HIDE", "1") or "1"
).strip().lower() not in {"0", "false", "no", "off"}
_CPP_QACTION_MEMCACHE_ENABLED = str(
    os.environ.get("FREECAD_COMMANDTAB_CPP_QACTION_MEMCACHE", "1") or "1"
).strip().lower() not in {"0", "false", "no", "off"}
_CPP_STARTUP_ORCHESTRATION_ENABLED = str(
    os.environ.get("FREECAD_COMMANDTAB_CPP_STARTUP_ORCHESTRATION", "1") or "1"
).strip().lower() not in {"0", "false", "no", "off"}
_NATIVE_EAGER_ALL_PANELS_ON_ACTIVATE = str(
    os.environ.get("FREECAD_COMMANDTAB_EAGER_ALL_PANELS", "0") or "0"
).strip().lower() not in {"0", "false", "no", "off"}
_NATIVE_STARTUP_PRELOAD_ALL_PANELS_ENABLED = str(
    os.environ.get("FREECAD_COMMANDTAB_STARTUP_PRELOAD_ALL_PANELS", "0") or "0"
).strip().lower() not in {"0", "false", "no", "off"}
try:
    _PAYLOAD_CACHE_MAX_ENTRIES = max(
        4,
        int(
            str(
                os.environ.get("FREECAD_COMMANDTAB_PAYLOAD_CACHE_MAX", "32")
                or "32"
            ).strip()
            or "32"
        ),
    )
except Exception:
    _PAYLOAD_CACHE_MAX_ENTRIES = 32
_NATIVE_WARMUP_DELAY_MS = int(os.environ.get("FREECAD_COMMANDTAB_WARMUP_DELAY_MS", "1800") or "1800")
_NATIVE_WARMUP_BATCH_DELAY_MS = int(os.environ.get("FREECAD_COMMANDTAB_WARMUP_BATCH_DELAY_MS", "110") or "110")
_NATIVE_STARTUP_REPAIR_DELAY_MS = int(
    os.environ.get("FREECAD_COMMANDTAB_STARTUP_REPAIR_DELAY_MS", "1200") or "1200"
)
_NATIVE_STARTUP_REPAIR_ENABLED = str(
    os.environ.get("FREECAD_COMMANDTAB_STARTUP_REPAIR_ENABLED", "0") or "0"
).strip().lower() in {"1", "true", "yes", "on"}
_NATIVE_METADATA_SCOPE_ALL_WORKBENCH_ICONS = str(
    os.environ.get("FREECAD_COMMANDTAB_METADATA_SCOPE_ALL_WORKBENCH_ICONS", "0") or "0"
).strip().lower() not in {"0", "false", "no", "off"}
_NATIVE_PERSISTENT_METADATA_BOOTSTRAP_ENABLED = str(
    os.environ.get("FREECAD_COMMANDTAB_PERSISTENT_METADATA_BOOTSTRAP", "0") or "0"
).strip().lower() in {"1", "true", "yes", "on"}
_NATIVE_METADATA_CACHE_VERSION = 36
_NATIVE_BOOTSTRAP_PAYLOAD_CACHE_VERSION = 2
_NATIVE_VARIANT_MENU_CACHE_VERSION = 1
_AVAILABLE_WORKBENCHES_CACHE_TTL_S = 0.45
_NATIVE_VARIANT_MENU_REPAIR_DELAY_MS = int(
    os.environ.get("FREECAD_COMMANDTAB_VARIANT_MENU_REPAIR_DELAY_MS", "900") or "900"
)
_NATIVE_WORKBENCH_ACTIVATION_DELAY_MS = int(
    os.environ.get("FREECAD_COMMANDTAB_WORKBENCH_ACTIVATION_DELAY_MS", "20") or "20"
)
_NATIVE_VARIANT_MENU_REPAIR_MAX_ATTEMPTS = max(
    1,
    int(os.environ.get("FREECAD_COMMANDTAB_VARIANT_MENU_REPAIR_MAX_ATTEMPTS", "4") or "4"),
)
_MAIN_WINDOW_PREFERENCES = App.ParamGet("User parameter:BaseApp/Preferences/MainWindow")
_COMMANDTAB_PREFERENCES = App.ParamGet("User parameter:BaseApp/Preferences/Mod/FreeCAD-CommandTab")
_GENERAL_PREFERENCES = App.ParamGet("User parameter:BaseApp/Preferences/General")
_THEME_MODE_KEY = "ThemeBootstrapMode"
_CACHE_LOCALE_SIGNATURE = ""
_LOADED_STRUCTURE_LANGUAGE = ""
_MACOS_FRENCH_TS_TRANSLATIONS: dict[str, dict[str, str]] | None = None
_MACOS_FRENCH_TS_CONTEXT_MESSAGES: dict[str, list[tuple[str, str]]] | None = None
_STATIC_COMMAND_VARIANT_MENUS: dict[str, tuple[str, ...]] = {
    "Sketcher_CompCreateArc": (
        "Sketcher_CreateArc",
        "Sketcher_Create3PointArc",
        "Sketcher_CreateArcOfEllipse",
        "Sketcher_CreateArcOfHyperbola",
        "Sketcher_CreateArcOfParabola",
    ),
    "Sketcher_CompCreateConic": (
        "Sketcher_CreateCircle",
        "Sketcher_Create3PointCircle",
        "Sketcher_CreateEllipseByCenter",
        "Sketcher_CreateEllipseBy3Points",
    ),
    "Sketcher_CompCreateRectangles": (
        "Sketcher_CreateRectangle",
        "Sketcher_CreateRectangle_Center",
        "Sketcher_CreateOblong",
    ),
    "Sketcher_CompCreateRegularPolygon": (
        "Sketcher_CreateTriangle",
        "Sketcher_CreateSquare",
        "Sketcher_CreatePentagon",
        "Sketcher_CreateHexagon",
        "Sketcher_CreateHeptagon",
        "Sketcher_CreateOctagon",
        "Sketcher_CreateRegularPolygon",
    ),
    "Sketcher_CompSlot": (
        "Sketcher_CreateSlot",
        "Sketcher_CreateArcSlot",
    ),
    "Sketcher_CompCreateBSpline": (
        "Sketcher_CreateBSpline",
        "Sketcher_CreatePeriodicBSpline",
        "Sketcher_CreateBSplineByInterpolation",
        "Sketcher_CreatePeriodicBSplineByInterpolation",
    ),
    "Sketcher_CompDimensionTools": (
        "Sketcher_Dimension",
        "Sketcher_ConstrainDistanceX",
        "Sketcher_ConstrainDistanceY",
        "Sketcher_ConstrainDistance",
        "Sketcher_ConstrainRadiam",
        "Sketcher_ConstrainRadius",
        "Sketcher_ConstrainDiameter",
        "Sketcher_ConstrainAngle",
        "Sketcher_ConstrainLock",
    ),
    "Sketcher_CompHorVer": (
        "Sketcher_ConstrainHorVer",
        "Sketcher_ConstrainHorizontal",
        "Sketcher_ConstrainVertical",
    ),
    "Sketcher_CompToggleConstraints": (
        "Sketcher_ToggleDrivingConstraint",
        "Sketcher_ToggleActiveConstraint",
    ),
    "Sketcher_CompCreateFillets": (
        "Sketcher_CreateFillet",
        "Sketcher_CreateChamfer",
    ),
    "Sketcher_CompCurveEdition": (
        "Sketcher_Trimming",
        "Sketcher_Split",
        "Sketcher_Extend",
    ),
    "Sketcher_CompExternal": (
        "Sketcher_Projection",
        "Sketcher_Intersection",
    ),
}


def _set_native_runtime_diagnostics(stage: str = "", error: str | None = None) -> None:
    global _NATIVE_RUNTIME_LAST_ERROR
    global _NATIVE_RUNTIME_LAST_STAGE

    if stage != "":
        _NATIVE_RUNTIME_LAST_STAGE = str(stage)
    if error is not None:
        _NATIVE_RUNTIME_LAST_ERROR = str(error or "")


def _coerce_dict(value) -> dict:
    return dict(value) if isinstance(value, dict) else {}


def _coerce_list(value) -> list:
    return list(value) if isinstance(value, list) else []


def _payload_has_command_variant_menus(payload: str) -> bool:
    try:
        parsed = json.loads(payload)
    except Exception:
        return False

    if isinstance(parsed, dict) is False:
        return False

    stack = [parsed]
    while len(stack) > 0:
        current = stack.pop()
        if isinstance(current, dict):
            command_type = str(current.get("type") or "").strip().lower()
            menu_commands = current.get("menuCommands")
            if command_type == "command" and isinstance(menu_commands, list) and len(menu_commands) > 0:
                return True
            for value in current.values():
                if isinstance(value, (dict, list)):
                    stack.append(value)
        elif isinstance(current, list):
            for value in current:
                if isinstance(value, (dict, list)):
                    stack.append(value)
    return False


def _clear_runtime_payload_caches() -> None:
    _MODEL_PAYLOAD_CACHE.clear()
    _BOOTSTRAP_PAYLOAD_CACHE.clear()
    _WORKBENCH_PAYLOAD_CACHE.clear()
    _WORKBENCH_BOOTSTRAP_CACHE.clear()
    _WORKBENCH_BOOTSTRAP_STATE_CACHE.clear()


def _is_separator_command_id(command_name: str) -> bool:
    normalized = str(command_name or "").strip().lower()
    if normalized == "":
        return False
    if normalized in {"0", "|", "-", "--", "---", "separator"}:
        return True
    return (
        normalized.startswith("separator_")
        or normalized.endswith("_separator")
        or "_separator_" in normalized
    )


def _bounded_cache_set(cache: dict, key, value, max_entries: int = _PAYLOAD_CACHE_MAX_ENTRIES) -> None:
    cache[key] = value
    if len(cache) <= max_entries:
        return
    try:
        while len(cache) > max_entries:
            cache.pop(next(iter(cache)))
    except Exception:
        cache.clear()
        cache[key] = value


def _global_text_visibility_for_size(size: str) -> bool:
    normalized_size = str(size or "").strip().lower()
    if normalized_size == "medium":
        return bool(getattr(Parameters_CommandTab, "SHOW_ICON_TEXT_MEDIUM", True))
    if normalized_size == "large":
        return bool(getattr(Parameters_CommandTab, "SHOW_ICON_TEXT_LARGE", True))
    return bool(getattr(Parameters_CommandTab, "SHOW_ICON_TEXT_SMALL", True))


def _resolved_command_text_visibility(command_data: dict, size: str) -> bool:
    visible = _global_text_visibility_for_size(size)
    # `textEnabled` is a legacy per-command flag written by the older Python
    # commandtab customizer. In the native commandtab it prevents the global
    # small/medium/large text preferences from taking effect on existing
    # layouts, so only the newer `textVisible` field is honored here.
    if "textVisible" in command_data:
        try:
            return bool(command_data.get("textVisible"))
        except Exception:
            return visible
    return visible


def _standard_functions_fallback():
    try:
        import Standard_Functions_CommandTab as StandardFunctions
    except Exception:
        return None
    return StandardFunctions


def _invalidate_native_model_caches() -> None:
    global _AVAILABLE_WORKBENCHES_CACHE_VALID
    global _AVAILABLE_WORKBENCHES_CACHE_TS

    _COMMAND_INFO_CACHE.clear()
    _COMMAND_ENTRY_CACHE.clear()
    _QT_ACTION_CACHE.clear()
    _QT_ACTION_CACHE_FAILURES.clear()
    _WORKBENCH_TITLE_CACHE.clear()
    _WORKBENCH_ICON_CACHE.clear()
    _BASE_MODEL_CACHE.clear()
    _WORKBENCH_PANEL_CACHE.clear()
    _WORKBENCH_PAYLOAD_CACHE.clear()
    _MODEL_PAYLOAD_CACHE.clear()
    _BOOTSTRAP_PAYLOAD_CACHE.clear()
    _WORKBENCH_BOOTSTRAP_CACHE.clear()
    _WORKBENCH_BOOTSTRAP_STATE_CACHE.clear()
    _AVAILABLE_WORKBENCHES_CACHE_VALID = False
    _AVAILABLE_WORKBENCHES_CACHE_TS = 0.0


def _native_theme_mode_setting() -> str:
    try:
        stored_value = str(_COMMANDTAB_PREFERENCES.GetString("NativeThemeMode") or "").strip().lower()
    except Exception:
        stored_value = ""
    if stored_value in ["dark", "light", "auto"]:
        return stored_value

    value = str(getattr(Parameters_CommandTab, "NATIVE_THEME_MODE", "auto")).strip().lower()
    if value in ["dark", "light", "auto"]:
        return value
    return "auto"


def _stored_theme_mode() -> str:
    stylesheet_name = _MAIN_WINDOW_PREFERENCES.GetString("StyleSheet").strip().lower()
    if "openlight" in stylesheet_name:
        return "light"
    if "opendark" in stylesheet_name:
        return "dark"

    main_window_theme = _MAIN_WINDOW_PREFERENCES.GetString("Theme").strip().lower()
    if "light" in main_window_theme:
        return "light"
    if "dark" in main_window_theme:
        return "dark"

    value = _COMMANDTAB_PREFERENCES.GetString(_THEME_MODE_KEY).strip().lower()
    if value in ["dark", "light"]:
        return value

    try:
        main_window = Gui.getMainWindow()
        if main_window is not None:
            palette = main_window.palette()
            probes = [palette.window().color(), palette.base().color(), palette.button().color()]
            valid_probes = [probe for probe in probes if probe.isValid()]
            if valid_probes:
                average = sum(probe.lightnessF() for probe in valid_probes) / float(len(valid_probes))
                return "dark" if average < 0.49 else "light"
    except Exception:
        pass

    return ""


def _native_theme_config() -> dict[str, object]:
    return {
        "themeMode": _native_theme_mode_setting(),
        "storedThemeMode": _stored_theme_mode(),
        "modernStyleEnabled": bool(
            getattr(Parameters_CommandTab, "MODERN_COMMANDTAB_STYLE_ENABLED", True)
        ),
    }


def _locale_signature_candidates() -> list[str]:
    candidates: list[str] = []

    def _append(value) -> None:
        value = str(value or "").strip()
        if value != "" and value not in candidates:
            candidates.append(value)

    try:
        _append(Gui.getLocale())
    except Exception:
        pass

    try:
        _append(_GENERAL_PREFERENCES.GetString("Language"))
    except Exception:
        pass

    try:
        system_locale = QLocale.system()
        _append(system_locale.name())
        _append(QLocale.languageToString(system_locale.language()))
    except Exception:
        pass

    return candidates


def _current_locale_signature() -> str:
    candidates = _locale_signature_candidates()
    for locale_name in candidates:
        if locale_name.strip().lower() not in {"", "c", "english", "en"}:
            return locale_name
    if candidates:
        return candidates[0]

    return "English"


def _is_macos_french_locale() -> bool:
    locale_signature = _current_locale_signature().strip().lower()
    return locale_signature in {"french", "fr", "fr_fr", "fr-fr"} or locale_signature.startswith("fr")


def _macos_french_ts_translations() -> dict[str, dict[str, str]]:
    global _MACOS_FRENCH_TS_TRANSLATIONS
    global _MACOS_FRENCH_TS_CONTEXT_MESSAGES

    if _MACOS_FRENCH_TS_TRANSLATIONS is not None:
        return _MACOS_FRENCH_TS_TRANSLATIONS

    translations: dict[str, dict[str, str]] = {}
    context_messages: dict[str, list[tuple[str, str]]] = {}
    translations_dir = Path(__file__).resolve().parent / "translations" / "fr"
    if not translations_dir.is_dir():
        _MACOS_FRENCH_TS_TRANSLATIONS = translations
        _MACOS_FRENCH_TS_CONTEXT_MESSAGES = context_messages
        return translations

    for ts_path in sorted(translations_dir.glob("*_fr.ts")):
        try:
            root = ET.parse(ts_path).getroot()
        except Exception:
            continue

        for context in root.findall("context"):
            context_name = (context.findtext("name") or "").strip()
            if context_name == "":
                continue
            context_translations = translations.setdefault(context_name, {})
            ordered_messages = context_messages.setdefault(context_name, [])
            for message in context.findall("message"):
                source_text = (message.findtext("source") or "").strip()
                translation = message.find("translation")
                if source_text == "" or translation is None:
                    continue
                if translation.attrib.get("type") in {"unfinished", "vanished", "obsolete"}:
                    continue
                translated_text = "".join(translation.itertext()).strip()
                if translated_text == "":
                    continue
                ordered_messages.append((source_text, translated_text))
                if translated_text != source_text:
                    context_translations.setdefault(source_text, translated_text)

    _MACOS_FRENCH_TS_TRANSLATIONS = translations
    _MACOS_FRENCH_TS_CONTEXT_MESSAGES = context_messages
    return translations


def _macos_french_ts_context_messages() -> dict[str, list[tuple[str, str]]]:
    global _MACOS_FRENCH_TS_CONTEXT_MESSAGES

    if _MACOS_FRENCH_TS_CONTEXT_MESSAGES is None:
        _macos_french_ts_translations()
    return _MACOS_FRENCH_TS_CONTEXT_MESSAGES or {}


def _macos_french_translation(context: str, source_text: str) -> str:
    if _is_macos_french_locale() is False:
        return ""
    context = str(context or "").strip()
    source_text = str(source_text or "").strip()
    if context == "" or source_text == "":
        return ""
    translated = _macos_french_ts_translations().get(context, {}).get(source_text, "")
    return _normalized_visible_text(translated)


_MACOS_FRENCH_NON_LABEL_SOURCES = {
    "arch",
    "assembly",
    "bim",
    "cam",
    "command",
    "commandgroup",
    "draft",
    "fem",
    "file",
    "help",
    "import",
    "material",
    "materials",
    "mesh",
    "openscad",
    "part",
    "partdesign",
    "points",
    "robot",
    "sketcher",
    "spreadsheet",
    "start",
    "std",
    "surface",
    "techdraw",
    "tux",
    "web",
    "workbench",
}


def _looks_like_translation_label_source(source_text: str) -> bool:
    source = _normalized_visible_text(source_text)
    if source == "":
        return False

    folded = source.casefold()
    if folded in _MACOS_FRENCH_NON_LABEL_SOURCES:
        return False
    if "\n" in source or "\r" in source or "\t" in source:
        return False
    if "<" in source or ">" in source:
        return False
    if "%" in source:
        return False
    if len(source) > 80:
        return False
    if source.endswith(".") or source.endswith(":"):
        return False

    words = source.split()
    if len(words) > 8:
        return False
    return True


def _macos_french_command_context_label(command_name: str) -> str:
    if _is_macos_french_locale() is False:
        return ""

    context_messages = _macos_french_ts_context_messages()
    contexts = _command_translation_contexts(command_name)
    contexts = [context for context in contexts if context not in {"CommandGroup", "Workbench"}]
    preferred_contexts = [context for context in contexts if context.startswith("Cmd")]
    preferred_contexts.extend(context for context in contexts if context not in preferred_contexts)

    for context in preferred_contexts:
        for source_text, translated_text in context_messages.get(context, []):
            translated = _normalized_visible_text(translated_text)
            if translated == "":
                continue
            if _looks_like_translation_label_source(source_text) is False:
                continue
            if _looks_like_technical_command_text(translated, command_name):
                continue
            return translated
    return ""


def _ensure_locale_caches_current() -> str:
    global _CACHE_LOCALE_SIGNATURE

    locale_signature = _current_locale_signature()
    if _CACHE_LOCALE_SIGNATURE != locale_signature:
        _invalidate_native_model_caches()
        _CACHE_LOCALE_SIGNATURE = locale_signature
    return locale_signature


def _native_theme_signature() -> tuple[str, str]:
    config = _native_theme_config()
    locale_signature = _ensure_locale_caches_current()
    theme_signature = "|".join(CommandTabTheme.theme_cache_signature())
    return (
        "|".join(
            [
                str(config.get("themeMode", "auto")),
                "modern" if bool(config.get("modernStyleEnabled", True)) else "classic",
                "original-freecad-icons-v5",
                f"locale:{locale_signature}",
                theme_signature,
            ]
        ),
        str(_MAIN_WINDOW_PREFERENCES.GetString("StyleSheet").strip().lower()),
    )


def _freecad_version_signature() -> str:
    try:
        version = App.Version()
    except Exception:
        version = ""

    if isinstance(version, (list, tuple)):
        parts = [str(part).strip() for part in version if str(part).strip() != ""]
        if parts:
            return "|".join(parts)

    version_text = str(version or "").strip()
    if version_text != "":
        return version_text

    for key in ["ExeVersion", "BuildVersion", "BuildRevision"]:
        try:
            value = str(App.ConfigGet(key) or "").strip()
        except Exception:
            value = ""
        if value != "":
            return f"{key}:{value}"
    return "unknown"


def _freecad_build_signature() -> str:
    parts: list[str] = []
    for key in [
        "BuildRevision",
        "BuildHash",
        "BuildVersion",
        "ExeVersion",
        "ExeVersionName",
    ]:
        try:
            value = str(App.ConfigGet(key) or "").strip()
        except Exception:
            value = ""
        if value != "":
            parts.append(f"{key}:{value}")
    return "|".join(parts)


def _native_metadata_environment_signature() -> str:
    workbench_names = sorted(
        {
            str(name).strip()
            for name in _available_workbenches().keys()
            if str(name).strip() != ""
        }
    )
    workbench_digest = ""
    if workbench_names:
        workbench_digest = hashlib.sha1(
            "|".join(workbench_names).encode("utf-8")
        ).hexdigest()

    return "|".join(
        [
            "env-v1",
            f"platform:{_host_platform_id()}",
            f"qt:{qVersion()}",
            f"freecad:{_freecad_version_signature()}",
            f"build:{_freecad_build_signature()}",
            f"workbenches:{len(workbench_names)}:{workbench_digest}",
        ]
    )


def is_native_mode_requested() -> bool:
    env_value = os.environ.get("FREECAD_COMMANDTAB_NATIVE", "").strip().lower()
    if env_value in ["1", "true", "yes", "on"]:
        return True
    return NATIVE_MODE_FLAG.exists()


def _is_native_warmup_enabled() -> bool:
    env_value = os.environ.get("FREECAD_COMMANDTAB_NATIVE_WARMUP", "").strip().lower()
    if env_value in ["0", "false", "no", "off"]:
        return False
    if env_value in ["1", "true", "yes", "on"]:
        return True
    return bool(getattr(Parameters_CommandTab, "NATIVE_COMMANDTAB_WARMUP", False))


def _is_cpp_bootstrap_pipeline_enabled() -> bool:
    return bool(_CPP_BOOTSTRAP_PIPELINE_ENABLED)


def _is_cpp_metadata_pipeline_enabled() -> bool:
    return bool(_CPP_METADATA_PIPELINE_ENABLED)


def _is_cpp_ui_hide_enabled() -> bool:
    return bool(_CPP_UI_HIDE_ENABLED)


def _is_cpp_qaction_memcache_enabled() -> bool:
    return bool(_CPP_QACTION_MEMCACHE_ENABLED)


def is_cpp_startup_orchestration_enabled() -> bool:
    return bool(_CPP_STARTUP_ORCHESTRATION_ENABLED)


def _is_native_eager_all_panels_on_activate() -> bool:
    return bool(_NATIVE_EAGER_ALL_PANELS_ON_ACTIVATE)


def _is_native_startup_preload_all_panels_enabled() -> bool:
    return bool(_NATIVE_STARTUP_PRELOAD_ALL_PANELS_ENABLED)


def _metadata_scope_all_workbench_icons_enabled() -> bool:
    return bool(_NATIVE_METADATA_SCOPE_ALL_WORKBENCH_ICONS)


def _is_persistent_metadata_bootstrap_enabled() -> bool:
    return bool(_NATIVE_PERSISTENT_METADATA_BOOTSTRAP_ENABLED)


def _should_preload_workbenches_for_metadata() -> bool:
    env_value = os.environ.get(
        "FREECAD_COMMANDTAB_PRELOAD_WORKBENCHES_FOR_METADATA", ""
    ).strip().lower()
    if env_value in {"1", "true", "yes", "on"}:
        return True
    if env_value in {"0", "false", "no", "off"}:
        return False
    return not _is_cpp_metadata_pipeline_enabled()


def _build_native_bootstrap_payload_safe(
    include_all_panels: bool = False,
) -> tuple[str, str, set[str], bool]:
    active_workbench, payload, loaded_workbenches = _build_native_bootstrap_payload(
        include_all_panels=include_all_panels
    )

    if include_all_panels is True:
        return active_workbench, payload, loaded_workbenches, True

    try:
        _structure_key, structure = _load_structure()
    except Exception:
        structure = {}
    if active_workbench == "" or _has_native_panels(structure, active_workbench) is False:
        # Do not load every workbench just because the active one has no
        # CommandTab panels. That fallback makes startup visibly slower.
        return active_workbench, payload, set(loaded_workbenches), False
    return active_workbench, payload, loaded_workbenches, False


def activate_native_commandtab(force: bool = False) -> bool:
    global _ACTIVE_CONTROLLER

    with StartupTrace.span("bridge.activate_native_commandtab", force=bool(force)):
        _set_native_runtime_diagnostics("activation_requested", None)
        if force is False and is_native_mode_requested() is False:
            _set_native_runtime_diagnostics("activation_skipped", "")
            return False

        if _ACTIVE_CONTROLLER is None:
            _ACTIVE_CONTROLLER = NativeCommandTabController()
        try:
            active = _ACTIVE_CONTROLLER.activate()
        except Exception as exc:
            _set_native_runtime_diagnostics("activation_failed", str(exc))
            try:
                _ACTIVE_CONTROLLER.close()
            except Exception:
                pass
            _ACTIVE_CONTROLLER = None
            raise
        _set_native_runtime_diagnostics("active" if active else "inactive", "" if active else _NATIVE_RUNTIME_LAST_ERROR)
        return active


def shutdown_native_commandtab() -> None:
    global _ACTIVE_CONTROLLER

    controller = _ACTIVE_CONTROLLER
    _ACTIVE_CONTROLLER = None
    if controller is None:
        return
    try:
        controller.close()
    except Exception:
        _logger.exception("native commandtab shutdown failed")
    _set_native_runtime_diagnostics("shutdown", "")


def native_runtime_status() -> dict[str, object]:
    platform_dir_candidates = _platform_directory_candidates()
    available_binary_dirs = []
    for directory in _binary_dir_candidates():
        if directory.exists():
            available_binary_dirs.append(str(directory))
    theme_config = _native_theme_config()
    return {
        "requested": is_native_mode_requested(),
        "active": is_native_commandtab_active(),
        "controllerLoaded": _ACTIVE_CONTROLLER is not None,
        "hostPlatform": _host_platform_id(),
        "hostPlatformSystemName": _host_platform_system_name(),
        "platformDirectoryCandidates": platform_dir_candidates,
        "qt_major": _qt_major_version(),
        "kind": _NATIVE_RUNTIME_KIND,
        "path": _NATIVE_RUNTIME_PATH,
        "libraryPlatform": _NATIVE_RUNTIME_LIBRARY_PLATFORM,
        "libraryPlatformSupported": _NATIVE_RUNTIME_LIBRARY_PLATFORM_SUPPORTED,
        "lastError": _NATIVE_RUNTIME_LAST_ERROR,
        "lastStage": _NATIVE_RUNTIME_LAST_STAGE,
        "binaryCandidates": [str(path) for path in _library_candidates()],
        "availableBinaryDirs": available_binary_dirs,
        "themeConfig": theme_config,
        "mainWindowTheme": _MAIN_WINDOW_PREFERENCES.GetString("Theme"),
        "mainWindowStyleSheet": _MAIN_WINDOW_PREFERENCES.GetString("StyleSheet"),
        "mainWindowOverlayStyleSheet": _MAIN_WINDOW_PREFERENCES.GetString("OverlayActiveStyleSheet"),
    }


def is_native_commandtab_active() -> bool:
    return _ACTIVE_CONTROLLER is not None and getattr(_ACTIVE_CONTROLLER, "_handle", None) not in [None, 0]


_NATIVE_THEME_OVERLAY_START = "/* FreeCAD CommandTab Native Python Theme Overlay: START */"
_NATIVE_THEME_OVERLAY_END = "/* FreeCAD CommandTab Native Python Theme Overlay: END */"


def _build_native_commandtab_theme_qss() -> str:
    """
    Keep Python-side theme overlay intentionally empty.

    The native C++ shell computes contrast per surface (tabs, panel footer,
    menu, tooltip, disabled text). A broad Python QSS overlay can override
    those colors and degrade readability.
    """
    return ""


def _strip_native_theme_overlay(existing_qss: str) -> str:
    text = str(existing_qss or "")
    if text == "":
        return ""

    # Remove marker-based overlay blocks (new format).
    while True:
        start = text.find(_NATIVE_THEME_OVERLAY_START)
        if start < 0:
            break
        end = text.find(_NATIVE_THEME_OVERLAY_END, start)
        if end < 0:
            text = text[:start].rstrip()
            break
        text = (text[:start] + text[end + len(_NATIVE_THEME_OVERLAY_END) :]).strip()

    # Remove legacy unmarked overlay generated by older bridge versions.
    legacy_pattern = re.compile(
        r"#FreeCADCommandTabNativeShell\s+QTabBar::tab,\s*"
        r"#FreeCADCommandTabNativeShell\s+CommandTabTabBar::tab\s*\{.*?"
        r"#FreeCADCommandTabNativeShell\s+QLabel#CommandTabPanelTitle\s*\{[^}]*\}",
        flags=re.DOTALL,
    )
    text = legacy_pattern.sub("", text).strip()
    return text


def apply_native_commandtab_theme_stylesheet() -> None:
    try:
        mw = Gui.getMainWindow()
        if mw is None:
            return
        native_shell = mw.findChild(QWidget, "FreeCADCommandTabNativeShell")
        if native_shell is None:
            return
        existing = native_shell.styleSheet() or ""
        cleaned = _strip_native_theme_overlay(existing)

        overlay = _build_native_commandtab_theme_qss().strip()
        if overlay != "":
            overlay_block = (
                f"{_NATIVE_THEME_OVERLAY_START}\n"
                f"{overlay}\n"
                f"{_NATIVE_THEME_OVERLAY_END}"
            )
            next_qss = cleaned
            if next_qss != "" and not next_qss.endswith("\n"):
                next_qss += "\n"
            next_qss += overlay_block
            native_shell.setStyleSheet(next_qss)
            return

        if cleaned != existing:
            native_shell.setStyleSheet(cleaned)
    except Exception:
        pass


def refresh_native_commandtab(force: bool = True, include_all_panels: bool = False) -> bool:
    if is_native_commandtab_active() is False:
        return False
    try:
        _ACTIVE_CONTROLLER.refresh(force=force, include_all_panels=include_all_panels)
        return True
    except Exception:
        return False


def enable_native_ui_hide_controller(enabled: bool = True) -> bool:
    if _is_cpp_ui_hide_enabled() is False:
        return False
    controller = _ACTIVE_CONTROLLER
    if controller is None:
        return False
    try:
        return bool(controller.set_ui_hide_controller_enabled(bool(enabled)))
    except Exception:
        return False


def _runtime_root() -> Path:
    root = Path(paths.user_cache_path("native-runtime"))
    root.mkdir(parents=True, exist_ok=True)
    return root


def _runtime_icons_dir() -> Path:
    icon_dir = _runtime_root() / "icons"
    icon_dir.mkdir(parents=True, exist_ok=True)
    return icon_dir


def _runtime_theme_stamp_path() -> Path:
    return _runtime_root() / "theme-signature.txt"


def _runtime_bootstrap_payload_cache_path() -> Path:
    return _runtime_root() / "bootstrap-payload-cache.json"


def _ensure_runtime_icon_cache_matches_theme() -> None:
    current_signature = "|".join(_native_theme_signature())
    stamp_path = _runtime_theme_stamp_path()
    previous_signature = ""
    if stamp_path.exists():
        try:
            previous_signature = stamp_path.read_text(encoding="utf-8").strip()
        except Exception:
            previous_signature = ""

    if previous_signature != current_signature:
        icon_dir = _runtime_icons_dir()
        if icon_dir.exists():
            shutil.rmtree(icon_dir, ignore_errors=True)
        icon_dir.mkdir(parents=True, exist_ok=True)
        workbench_icon_dir = _runtime_workbench_icons_dir()
        if workbench_icon_dir.exists():
            shutil.rmtree(workbench_icon_dir, ignore_errors=True)
        workbench_icon_dir.mkdir(parents=True, exist_ok=True)
        qt_action_icon_dir = _qt_action_icons_dir()
        if qt_action_icon_dir.exists():
            shutil.rmtree(qt_action_icon_dir, ignore_errors=True)
        qt_action_icon_dir.mkdir(parents=True, exist_ok=True)
        persistent_icon_dir = _persistent_icon_export_dir()
        if persistent_icon_dir.exists():
            shutil.rmtree(persistent_icon_dir, ignore_errors=True)
        persistent_icon_dir.mkdir(parents=True, exist_ok=True)
        _QT_ACTION_CACHE.clear()
        _QT_ACTION_CACHE_FAILURES.clear()
        _WORKBENCH_ICON_CACHE.clear()
        _invalidate_native_model_caches()
        stamp_path.write_text(current_signature, encoding="utf-8")


def _native_settings_state_json() -> str:
    commandtab_preferences = App.ParamGet("User parameter:BaseApp/Preferences/Mod/FreeCAD-CommandTab")
    commandtab_runtime_settings = App.ParamGet(
        "User parameter:BaseApp/Preferences/Mod/FreeCAD-CommandTab/Settings"
    )
    commandtab_contents = []
    runtime_settings_contents = []
    try:
        commandtab_contents = list(commandtab_preferences.GetContents())
    except Exception:
        commandtab_contents = []
    try:
        runtime_settings_contents = list(commandtab_runtime_settings.GetContents())
    except Exception:
        runtime_settings_contents = []

    def _bool_setting(name: str, default: bool) -> bool:
        if any(setting[0] == "Boolean" and setting[1] == name for setting in commandtab_contents):
            try:
                return bool(commandtab_preferences.GetBool(name))
            except Exception:
                return bool(default)
        return bool(default)

    def _runtime_bool_setting(name: str, default: bool) -> bool:
        if any(setting[0] == "Boolean" and setting[1] == name for setting in runtime_settings_contents):
            try:
                return bool(commandtab_runtime_settings.GetBool(name))
            except Exception:
                return bool(default)
        return _bool_setting(name, default)

    def _int_setting(name: str, default: int, minimum: int, maximum: int) -> int:
        exists = any(len(setting) > 1 and setting[1] == name for setting in commandtab_contents)
        if not exists:
            return int(default)
        try:
            value = int(commandtab_preferences.GetInt(name))
            return int(max(minimum, min(maximum, value)))
        except Exception:
            return int(default)

    def _string_setting(name: str, default: str) -> str:
        exists = any(len(setting) > 1 and setting[1] == name for setting in commandtab_contents)
        if not exists:
            return str(default)
        try:
            value = str(commandtab_preferences.GetString(name) or "").strip().lower()
        except Exception:
            value = str(default).strip().lower()
        return value if value else str(default).strip().lower()

    def _color_setting(name: str, default: str = "") -> str:
        exists = any(len(setting) > 1 and setting[1] == name for setting in commandtab_contents)
        if not exists:
            return str(default or "").strip()
        try:
            raw_value = str(commandtab_preferences.GetString(name) or "").strip()
        except Exception:
            raw_value = str(default or "").strip()
        if raw_value == "":
            return str(default or "").strip()
        color = QColor(raw_value)
        if not color.isValid():
            return str(default or "").strip()
        return str(color.name())

    def _runtime_string_setting(name: str, default: str) -> str:
        exists = any(len(setting) > 1 and setting[1] == name for setting in runtime_settings_contents)
        if not exists:
            return _string_setting(name, default)
        try:
            value = str(commandtab_runtime_settings.GetString(name) or "").strip().lower()
        except Exception:
            value = str(default).strip().lower()
        return value if value else str(default).strip().lower()

    def _runtime_color_setting(name: str, default: str = "") -> str:
        exists = any(len(setting) > 1 and setting[1] == name for setting in runtime_settings_contents)
        if not exists:
            return _color_setting(name, default)
        try:
            raw_value = str(commandtab_runtime_settings.GetString(name) or "").strip()
        except Exception:
            raw_value = str(default or "").strip()
        if raw_value == "":
            return str(default or "").strip()
        color = QColor(raw_value)
        if not color.isValid():
            return str(default or "").strip()
        return str(color.name())

    legacy_display_scale_percent = _int_setting(
        "NativeDisplayScalePercent", 100, 60, 140
    )
    header_scale_percent = _int_setting(
        "NativeHeaderScalePercent", legacy_display_scale_percent, 60, 140
    )
    commandtab_scale_percent = _int_setting(
        "NativeCommandTabScalePercent", legacy_display_scale_percent, 60, 140
    )

    return json.dumps(
        {
            "preferNativeCommandTab": bool(getattr(Parameters_CommandTab, "PREFER_NATIVE_COMMANDTAB", True)),
            "nativeCommandTabWarmup": bool(getattr(Parameters_CommandTab, "NATIVE_COMMANDTAB_WARMUP", False)),
            "modernCommandTabStyleEnabled": bool(getattr(Parameters_CommandTab, "MODERN_COMMANDTAB_STYLE_ENABLED", True)),
            "applyOndselDefaults": bool(getattr(Parameters_CommandTab, "APPLY_ONDSEL_DEFAULTS", True)),
            "hideMenuBarInNativeMode": bool(getattr(Parameters_CommandTab, "HIDE_MENUBAR_IN_NATIVE_MODE", True)),
            "nativeThemeMode": _string_setting("NativeThemeMode", _native_theme_mode_setting()),
            "ribbonSurfaceStyle": _runtime_string_setting("RibbonSurfaceStyle", "glass"),
            "ribbonSurfaceTransparent": _runtime_bool_setting("RibbonSurfaceTransparent", False),
            "compactPanelLayout": _bool_setting("NativeCompactPanelLayout", True),
            "panelDropdownModeEnabled": _bool_setting("NativePanelDropdownModeEnabled", False),
            "panelDropdownPrimaryRecent": _bool_setting("NativePanelDropdownPrimaryRecent", False),
            "panelDropdownRecentToolCount": _int_setting(
                "NativePanelDropdownRecentToolCount", 2, 0, 9
            ),
            "panelDropdownPopupColumns": _int_setting(
                "NativePanelDropdownPopupColumns", 4, 2, 8
            ),
            "panelDropdownPopupIconSize": _int_setting(
                "NativePanelDropdownPopupIconSize", 20, 12, 48
            ),
            "displayScalePercent": commandtab_scale_percent,
            "headerScalePercent": header_scale_percent,
            "commandtabScalePercent": commandtab_scale_percent,
            "panelDropdownPopupShowText": _bool_setting(
                "NativePanelDropdownPopupShowText", False
            ),
            "compactPanelSpacing": _int_setting("NativeCompactPanelSpacing", 1, 0, 6),
            "compactButtonPadding": _int_setting("NativeCompactButtonPadding", 3, 1, 8),
            "buttonBordersVisible": _bool_setting("NativeButtonBordersVisible", True),
            "showIconTextSmall": bool(getattr(Parameters_CommandTab, "SHOW_ICON_TEXT_SMALL", True)),
            "showIconTextMedium": bool(getattr(Parameters_CommandTab, "SHOW_ICON_TEXT_MEDIUM", True)),
            "showIconTextLarge": bool(getattr(Parameters_CommandTab, "SHOW_ICON_TEXT_LARGE", True)),
            "iconOnlySizeSmall": _int_setting("NativeIconOnlySize_Small", 18, 12, 64),
            "iconOnlySizeMedium": _int_setting("NativeIconOnlySize_Medium", 20, 12, 64),
            "iconOnlySizeLarge": _int_setting("NativeIconOnlySize_Large", 22, 12, 64),
            "showSketcherGrid": _bool_setting("ShowSketcherGrid", True),
            "snapSketcherGrid": _bool_setting("SnapSketcherGrid", True),
            "customMainColorsEnabled": _bool_setting("NativeCustomMainColorsEnabled", False),
            "customMainBackgroundColor": _color_setting("NativeCustomMainBackgroundColor", ""),
            "customMainTextColor": _color_setting("NativeCustomMainTextColor", ""),
            "customRibbonPrimaryColor": _color_setting("NativeCustomRibbonPrimaryColor", ""),
            "customRibbonSecondaryColor": _color_setting("NativeCustomRibbonSecondaryColor", ""),
            "customRibbonAccentColor": _color_setting("NativeCustomRibbonAccentColor", ""),
            "ribbonAutoHide": _bool_setting("RibbonAutoHide", False),
            "ribbonAutoHideDelayMs": _int_setting("RibbonAutoHideDelayMs", 1500, 300, 10000),
            "ribbonHoverTab": _bool_setting("RibbonHoverTab", False),
            "tabClickPopupMode": _bool_setting(
                "TabClickPopupMode",
                bool(getattr(Parameters_CommandTab, "TAB_CLICK_POPUP_MODE", False)),
            ),
            "viewportColorsEnabled": _runtime_bool_setting("ViewportColorsEnabled", False),
            "viewportBackgroundStyle": _runtime_string_setting("ViewportBackgroundStyle", "linear"),
            "viewportBgTopColor": _runtime_color_setting("ViewportBgTopColor", ""),
            "viewportBgMidColor": _runtime_color_setting("ViewportBgMidColor", ""),
            "viewportBgBottomColor": _runtime_color_setting("ViewportBgBottomColor", ""),
            "viewportBgAccentColor": _runtime_color_setting("ViewportBgAccentColor", ""),
            "gridColorEnabled": _runtime_bool_setting("GridColorEnabled", False),
            "viewportGridColor": _runtime_color_setting("ViewportGridColor", ""),
        },
        ensure_ascii=True,
        separators=(",", ":"),
    )


def _set_native_icon_text_visibility(size: str, visible: bool) -> None:
    mapping = {
        "small": ("ShowIconText_Small", "SHOW_ICON_TEXT_SMALL"),
        "medium": ("ShowIconText_Medium", "SHOW_ICON_TEXT_MEDIUM"),
        "large": ("ShowIconText_Large", "SHOW_ICON_TEXT_LARGE"),
    }
    setting = mapping.get(str(size or "").strip().lower())
    if setting is None:
        return

    setting_name, parameter_name = setting
    Parameters_CommandTab.Settings.SetBoolSetting(setting_name, bool(visible))
    setattr(Parameters_CommandTab, parameter_name, bool(visible))
    App.saveParameter()
    _invalidate_native_model_caches()
    settings_json = _native_settings_state_json()
    if _ACTIVE_CONTROLLER is not None:
        try:
            if _ACTIVE_CONTROLLER.apply_settings(settings_json):
                return
        except Exception:
            _logger.exception("apply_settings failed during icon-text visibility toggle, falling back to full refresh")
    refresh_native_commandtab(force=True, include_all_panels=False)


def _apply_viewport_colors(
    viewport_enabled: bool,
    background_style: str,
    bg_top_color: str,
    bg_mid_color: str,
    bg_bottom_color: str,
    bg_accent_color: str,
    grid_enabled: bool,
    grid_color: str,
) -> None:
    def _hex_to_rgb(hex_str: str):
        h = hex_str.strip().lstrip("#")
        if len(h) in (6, 8):
            return int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16)
        raise ValueError(f"Invalid hex color: {hex_str!r}")

    def _pack(hex_str: str, default: int) -> int:
        try:
            r, g, b = _hex_to_rgb(hex_str)
            return (r << 24) | (g << 16) | (b << 8) | 0xFF
        except Exception:
            return default

    def _unpack_to_hex(value: int, default_hex: str) -> str:
        try:
            unsigned_value = int(value) & 0xFFFFFFFF
            r = (unsigned_value >> 24) & 0xFF
            g = (unsigned_value >> 16) & 0xFF
            b = (unsigned_value >> 8) & 0xFF
            return f"#{r:02x}{g:02x}{b:02x}"
        except Exception:
            return default_hex

    def _normalize_style(raw_style: str) -> str:
        style = str(raw_style or "").strip().lower()
        if style in ["solid", "linear", "tricolor", "radial", "quad"]:
            return style
        return "linear"

    def _blend_hex(primary: str, secondary: str, ratio: float = 0.5) -> str:
        try:
            p_r, p_g, p_b = _hex_to_rgb(primary)
            s_r, s_g, s_b = _hex_to_rgb(secondary)
        except Exception:
            return primary
        clamped_ratio = max(0.0, min(1.0, float(ratio)))
        r = int(round((p_r * (1.0 - clamped_ratio)) + (s_r * clamped_ratio)))
        g = int(round((p_g * (1.0 - clamped_ratio)) + (s_g * clamped_ratio)))
        b = int(round((p_b * (1.0 - clamped_ratio)) + (s_b * clamped_ratio)))
        return f"#{r:02x}{g:02x}{b:02x}"

    ct = App.ParamGet("User parameter:BaseApp/Preferences/Mod/FreeCAD-CommandTab/Settings")
    view_params = App.ParamGet("User parameter:BaseApp/Preferences/View")

    if viewport_enabled:
        # Save originals once so we can restore on disable
        if not ct.GetBool("ViewportBgBacked", False):
            ct.SetUnsigned("OrigBgColor",  view_params.GetUnsigned("BackgroundColor",  0x00000000))
            ct.SetUnsigned("OrigBgColor2", view_params.GetUnsigned("BackgroundColor2", 0x00000000))
            ct.SetUnsigned("OrigBgColor3", view_params.GetUnsigned("BackgroundColor3", 0x00000000))
            ct.SetUnsigned("OrigBgColor4", view_params.GetUnsigned("BackgroundColor4", 0x00000000))
            ct.SetBool("OrigBgSimple", view_params.GetBool("Simple", False))
            ct.SetBool("OrigBgGradient", view_params.GetBool("Gradient", False))
            ct.SetBool("OrigBgRadialGradient", view_params.GetBool("RadialGradient", False))
            ct.SetBool("OrigBgUseMid", view_params.GetBool("UseBackgroundColorMid", False))
            ct.SetBool("ViewportBgBacked", True)

        style = _normalize_style(background_style)
        default_top = _unpack_to_hex(view_params.GetUnsigned("BackgroundColor2", 0x19283300), "#192833")
        default_bottom = _unpack_to_hex(view_params.GetUnsigned("BackgroundColor", 0x0a0e1200), "#0a0e12")
        default_middle = _unpack_to_hex(view_params.GetUnsigned("BackgroundColor3", 0x131f2900), "#131f29")
        default_accent = _unpack_to_hex(view_params.GetUnsigned("BackgroundColor4", 0x3f5a7300), "#3f5a73")
        top = bg_top_color or default_top
        bottom = bg_bottom_color or default_bottom
        middle = bg_mid_color or default_middle or _blend_hex(top, bottom, 0.5)
        accent = bg_accent_color or default_accent or top

        if style == "solid":
            view_params.SetBool("Simple", True)
            view_params.SetBool("Gradient", False)
            view_params.SetBool("RadialGradient", False)
            view_params.SetBool("UseBackgroundColorMid", False)
            view_params.SetUnsigned("BackgroundColor", _pack(top, 0x19283300))
            view_params.SetUnsigned("BackgroundColor2", _pack(top, 0x19283300))
            view_params.SetUnsigned("BackgroundColor3", _pack(top, 0x19283300))
            view_params.SetUnsigned("BackgroundColor4", _pack(top, 0x19283300))
        elif style == "tricolor":
            view_params.SetBool("Simple", False)
            view_params.SetBool("Gradient", True)
            view_params.SetBool("RadialGradient", False)
            view_params.SetBool("UseBackgroundColorMid", True)
            view_params.SetUnsigned("BackgroundColor", _pack(bottom, 0x0a0e1200))
            view_params.SetUnsigned("BackgroundColor2", _pack(top, 0x19283300))
            view_params.SetUnsigned("BackgroundColor3", _pack(middle, 0x131f2900))
            view_params.SetUnsigned("BackgroundColor4", _pack(accent, 0x19283300))
        elif style == "radial":
            view_params.SetBool("Simple", False)
            view_params.SetBool("Gradient", True)
            view_params.SetBool("RadialGradient", True)
            view_params.SetBool("UseBackgroundColorMid", True)
            view_params.SetUnsigned("BackgroundColor", _pack(bottom, 0x0a0e1200))
            view_params.SetUnsigned("BackgroundColor2", _pack(top, 0x19283300))
            view_params.SetUnsigned("BackgroundColor3", _pack(middle, 0x131f2900))
            view_params.SetUnsigned("BackgroundColor4", _pack(accent, 0x19283300))
        elif style == "quad":
            view_params.SetBool("Simple", False)
            view_params.SetBool("Gradient", True)
            view_params.SetBool("RadialGradient", False)
            view_params.SetBool("UseBackgroundColorMid", True)
            view_params.SetUnsigned("BackgroundColor", _pack(bottom, 0x0a0e1200))
            view_params.SetUnsigned("BackgroundColor2", _pack(top, 0x19283300))
            view_params.SetUnsigned("BackgroundColor3", _pack(middle, 0x131f2900))
            view_params.SetUnsigned("BackgroundColor4", _pack(accent, 0x3f5a7300))
        else:
            # "linear"
            view_params.SetBool("Simple", False)
            view_params.SetBool("Gradient", True)
            view_params.SetBool("RadialGradient", False)
            view_params.SetBool("UseBackgroundColorMid", False)
            view_params.SetUnsigned("BackgroundColor", _pack(bottom, 0x0a0e1200))
            view_params.SetUnsigned("BackgroundColor2", _pack(top, 0x19283300))
            view_params.SetUnsigned("BackgroundColor3", _pack(middle, 0x131f2900))
            view_params.SetUnsigned("BackgroundColor4", _pack(accent, 0x19283300))
    elif not viewport_enabled and ct.GetBool("ViewportBgBacked", False):
        # Restore originals
        view_params.SetUnsigned("BackgroundColor",  ct.GetUnsigned("OrigBgColor",  0x00000000))
        view_params.SetUnsigned("BackgroundColor2", ct.GetUnsigned("OrigBgColor2", 0x00000000))
        view_params.SetUnsigned("BackgroundColor3", ct.GetUnsigned("OrigBgColor3", 0x00000000))
        view_params.SetUnsigned("BackgroundColor4", ct.GetUnsigned("OrigBgColor4", 0x00000000))
        view_params.SetBool("Simple", ct.GetBool("OrigBgSimple", False))
        view_params.SetBool("Gradient", ct.GetBool("OrigBgGradient", False))
        view_params.SetBool("RadialGradient", ct.GetBool("OrigBgRadialGradient", False))
        view_params.SetBool("UseBackgroundColorMid", ct.GetBool("OrigBgUseMid", False))
        ct.SetBool("ViewportBgBacked", False)

    try:
        draft_params = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")
    except Exception as exc:
        draft_params = None
        _logger.warning("Unable to access Draft preferences for grid color update: %s", exc)

    if grid_enabled and grid_color and draft_params is not None:
        try:
            if ct.GetBool("GridColorBacked", False) is False:
                ct.SetUnsigned("OrigGridColor", draft_params.GetUnsigned("gridColor", 0x40404000))
                ct.SetBool("GridColorBacked", True)
            draft_params.SetUnsigned("gridColor", _pack(grid_color, 0x40404000))
        except Exception as exc:
            _logger.warning("Failed to apply Draft grid color override: %s", exc)
    elif not grid_enabled and draft_params is not None and ct.GetBool("GridColorBacked", False):
        try:
            draft_params.SetUnsigned("gridColor", ct.GetUnsigned("OrigGridColor", 0x40404000))
            ct.SetBool("GridColorBacked", False)
        except Exception as exc:
            _logger.warning("Failed to restore Draft grid color: %s", exc)

    try:
        import FreeCADGui as Gui
        if hasattr(Gui, "updateGui"):
            Gui.updateGui()
    except Exception:
        pass


def _apply_native_preferences_payload(encoded_payload: str) -> None:
    payload = _decode_base64_json_payload(encoded_payload)

    def _normalized_color(value, default: str = "") -> str:
        raw = str(value or "").strip()
        if raw == "":
            return str(default or "").strip()
        parsed = QColor(raw)
        if not parsed.isValid():
            return str(default or "").strip()
        return str(parsed.name())

    def _clamped_int(value, default: int, minimum: int, maximum: int) -> int:
        try:
            parsed = int(value)
        except Exception:
            parsed = int(default)
        return int(max(minimum, min(maximum, parsed)))

    previous_modern_commandtab_style_enabled = bool(
        getattr(Parameters_CommandTab, "MODERN_COMMANDTAB_STYLE_ENABLED", True)
    )
    previous_native_theme_mode = str(
        getattr(Parameters_CommandTab, "NATIVE_THEME_MODE", "auto")
    ).strip().lower()

    prefer_native_commandtab = bool(payload.get("preferNativeCommandTab", True))
    native_commandtab_warmup = bool(payload.get("nativeCommandTabWarmup", False))
    modern_commandtab_style_enabled = bool(payload.get("modernCommandTabStyleEnabled", True))
    apply_ondsel_defaults = bool(payload.get("applyOndselDefaults", True))
    hide_menu_bar_in_native_mode = bool(
        payload.get(
            "hideMenuBarInNativeMode",
            bool(getattr(Parameters_CommandTab, "HIDE_MENUBAR_IN_NATIVE_MODE", True)),
        )
    )
    native_theme_mode = str(payload.get("nativeThemeMode") or "auto").strip().lower()
    compact_panel_layout = bool(payload.get("compactPanelLayout", True))
    panel_dropdown_mode_enabled = bool(payload.get("panelDropdownModeEnabled", False))
    panel_dropdown_primary_recent = bool(payload.get("panelDropdownPrimaryRecent", False))
    panel_dropdown_recent_tool_count = _clamped_int(
        payload.get("panelDropdownRecentToolCount", 2), 2, 0, 9
    )
    panel_dropdown_popup_columns = _clamped_int(
        payload.get("panelDropdownPopupColumns", 4), 4, 2, 8
    )
    panel_dropdown_popup_icon_size = _clamped_int(
        payload.get("panelDropdownPopupIconSize", 20), 20, 12, 48
    )
    display_scale_percent = _clamped_int(
        payload.get("displayScalePercent", 100), 100, 60, 140
    )
    header_scale_percent = _clamped_int(
        payload.get("headerScalePercent", display_scale_percent),
        display_scale_percent,
        60,
        140,
    )
    commandtab_scale_percent = _clamped_int(
        payload.get("commandtabScalePercent", display_scale_percent),
        display_scale_percent,
        60,
        140,
    )
    panel_dropdown_popup_show_text = bool(payload.get("panelDropdownPopupShowText", False))
    compact_panel_spacing = _clamped_int(payload.get("compactPanelSpacing", 1), 1, 0, 6)
    compact_button_padding = _clamped_int(payload.get("compactButtonPadding", 3), 3, 1, 8)
    button_borders_visible = bool(payload.get("buttonBordersVisible", True))
    show_icon_text_small = bool(payload.get("showIconTextSmall", True))
    show_icon_text_medium = bool(payload.get("showIconTextMedium", True))
    show_icon_text_large = bool(payload.get("showIconTextLarge", True))
    icon_only_size_small = _clamped_int(payload.get("iconOnlySizeSmall", 18), 18, 12, 64)
    icon_only_size_medium = _clamped_int(payload.get("iconOnlySizeMedium", 20), 20, 12, 64)
    icon_only_size_large = _clamped_int(payload.get("iconOnlySizeLarge", 22), 22, 12, 64)
    show_sketcher_grid = bool(payload.get("showSketcherGrid", True))
    snap_sketcher_grid = bool(payload.get("snapSketcherGrid", True))
    custom_main_colors_enabled = bool(payload.get("customMainColorsEnabled", False))
    custom_main_background_color = _normalized_color(payload.get("customMainBackgroundColor", ""), "")
    custom_main_text_color = _normalized_color(payload.get("customMainTextColor", ""), "")
    custom_ribbon_primary_color = _normalized_color(payload.get("customRibbonPrimaryColor", ""), "")
    custom_ribbon_secondary_color = _normalized_color(payload.get("customRibbonSecondaryColor", ""), "")
    custom_ribbon_accent_color = _normalized_color(payload.get("customRibbonAccentColor", ""), "")
    ribbon_surface_style = str(payload.get("ribbonSurfaceStyle", "glass") or "glass").strip().lower()
    if ribbon_surface_style not in ["mat", "normal", "glass"]:
        ribbon_surface_style = "glass"
    ribbon_surface_transparent = bool(payload.get("ribbonSurfaceTransparent", False))
    ribbon_auto_hide = bool(payload.get("ribbonAutoHide", False))
    ribbon_auto_hide_delay_ms = int(payload.get("ribbonAutoHideDelayMs", 1500))
    ribbon_hover_tab = bool(payload.get("ribbonHoverTab", False))
    tab_click_popup_mode = bool(payload.get("tabClickPopupMode", False))
    viewport_colors_enabled = bool(payload.get("viewportColorsEnabled", False))
    viewport_background_style = str(
        payload.get("viewportBackgroundStyle", "linear") or "linear"
    ).strip().lower()
    if viewport_background_style not in ["solid", "linear", "tricolor", "radial", "quad"]:
        viewport_background_style = "linear"
    viewport_bg_top_color = _normalized_color(payload.get("viewportBgTopColor", ""), "")
    viewport_bg_mid_color = _normalized_color(payload.get("viewportBgMidColor", ""), "")
    viewport_bg_bottom_color = _normalized_color(payload.get("viewportBgBottomColor", ""), "")
    viewport_bg_accent_color = _normalized_color(payload.get("viewportBgAccentColor", ""), "")
    grid_color_enabled = bool(payload.get("gridColorEnabled", False))
    viewport_grid_color = _normalized_color(payload.get("viewportGridColor", ""), "")
    if native_theme_mode not in ["auto", "dark", "light"]:
        native_theme_mode = "auto"

    Parameters_CommandTab.Settings.SetBoolSetting("PreferNativeCommandTab", prefer_native_commandtab)
    Parameters_CommandTab.Settings.SetBoolSetting("NativeCommandTabWarmup", native_commandtab_warmup)
    Parameters_CommandTab.Settings.SetBoolSetting(
        "ModernCommandTabStyleEnabled", modern_commandtab_style_enabled
    )
    Parameters_CommandTab.Settings.SetBoolSetting(
        "ApplyOndselDefaults", apply_ondsel_defaults
    )
    Parameters_CommandTab.Settings.SetBoolSetting(
        "HideMenuBarInNativeMode", hide_menu_bar_in_native_mode
    )
    Parameters_CommandTab.Settings.SetStringSetting("NativeThemeMode", native_theme_mode)
    Parameters_CommandTab.Settings.SetBoolSetting("NativeCompactPanelLayout", compact_panel_layout)
    Parameters_CommandTab.Settings.SetBoolSetting(
        "NativePanelDropdownModeEnabled", panel_dropdown_mode_enabled
    )
    Parameters_CommandTab.Settings.SetBoolSetting(
        "NativePanelDropdownPrimaryRecent", panel_dropdown_primary_recent
    )
    Parameters_CommandTab.Settings.SetIntSetting(
        "NativePanelDropdownRecentToolCount", panel_dropdown_recent_tool_count
    )
    Parameters_CommandTab.Settings.SetIntSetting(
        "NativePanelDropdownPopupColumns", panel_dropdown_popup_columns
    )
    Parameters_CommandTab.Settings.SetIntSetting(
        "NativePanelDropdownPopupIconSize", panel_dropdown_popup_icon_size
    )
    Parameters_CommandTab.Settings.SetIntSetting("NativeDisplayScalePercent", commandtab_scale_percent)
    Parameters_CommandTab.Settings.SetIntSetting("NativeHeaderScalePercent", header_scale_percent)
    Parameters_CommandTab.Settings.SetIntSetting("NativeCommandTabScalePercent", commandtab_scale_percent)
    Parameters_CommandTab.Settings.SetBoolSetting(
        "NativePanelDropdownPopupShowText", panel_dropdown_popup_show_text
    )
    Parameters_CommandTab.Settings.SetIntSetting("NativeCompactPanelSpacing", compact_panel_spacing)
    Parameters_CommandTab.Settings.SetIntSetting("NativeCompactButtonPadding", compact_button_padding)
    Parameters_CommandTab.Settings.SetBoolSetting("NativeButtonBordersVisible", button_borders_visible)
    Parameters_CommandTab.Settings.SetBoolSetting("ShowIconText_Small", show_icon_text_small)
    Parameters_CommandTab.Settings.SetBoolSetting("ShowIconText_Medium", show_icon_text_medium)
    Parameters_CommandTab.Settings.SetBoolSetting("ShowIconText_Large", show_icon_text_large)
    Parameters_CommandTab.Settings.SetIntSetting("NativeIconOnlySize_Small", icon_only_size_small)
    Parameters_CommandTab.Settings.SetIntSetting("NativeIconOnlySize_Medium", icon_only_size_medium)
    Parameters_CommandTab.Settings.SetIntSetting("NativeIconOnlySize_Large", icon_only_size_large)
    App.ParamGet("User parameter:BaseApp/Preferences/Mod/FreeCAD-CommandTab").SetBool(
        "ShowSketcherGrid", show_sketcher_grid
    )
    App.ParamGet("User parameter:BaseApp/Preferences/Mod/FreeCAD-CommandTab").SetBool(
        "SnapSketcherGrid", snap_sketcher_grid
    )
    App.ParamGet("User parameter:BaseApp/Preferences/Mod/FreeCAD-CommandTab").SetBool(
        "NativeCustomMainColorsEnabled", custom_main_colors_enabled
    )
    App.ParamGet("User parameter:BaseApp/Preferences/Mod/FreeCAD-CommandTab").SetString(
        "NativeCustomMainBackgroundColor", custom_main_background_color
    )
    App.ParamGet("User parameter:BaseApp/Preferences/Mod/FreeCAD-CommandTab").SetString(
        "NativeCustomMainTextColor", custom_main_text_color
    )
    App.ParamGet("User parameter:BaseApp/Preferences/Mod/FreeCAD-CommandTab").SetString(
        "NativeCustomRibbonPrimaryColor", custom_ribbon_primary_color
    )
    App.ParamGet("User parameter:BaseApp/Preferences/Mod/FreeCAD-CommandTab").SetString(
        "NativeCustomRibbonSecondaryColor", custom_ribbon_secondary_color
    )
    App.ParamGet("User parameter:BaseApp/Preferences/Mod/FreeCAD-CommandTab").SetString(
        "NativeCustomRibbonAccentColor", custom_ribbon_accent_color
    )
    _ct_settings = App.ParamGet("User parameter:BaseApp/Preferences/Mod/FreeCAD-CommandTab/Settings")
    _ct_settings.SetString("RibbonSurfaceStyle", ribbon_surface_style)
    _ct_settings.SetBool("RibbonSurfaceTransparent", ribbon_surface_transparent)
    _ct_root_settings = App.ParamGet("User parameter:BaseApp/Preferences/Mod/FreeCAD-CommandTab")
    _ct_root_settings.SetString("RibbonSurfaceStyle", ribbon_surface_style)
    _ct_root_settings.SetBool("RibbonSurfaceTransparent", ribbon_surface_transparent)

    Parameters_CommandTab.PREFER_NATIVE_COMMANDTAB = prefer_native_commandtab
    Parameters_CommandTab.NATIVE_COMMANDTAB_WARMUP = native_commandtab_warmup
    Parameters_CommandTab.MODERN_COMMANDTAB_STYLE_ENABLED = modern_commandtab_style_enabled
    Parameters_CommandTab.APPLY_ONDSEL_DEFAULTS = apply_ondsel_defaults
    Parameters_CommandTab.HIDE_MENUBAR_IN_NATIVE_MODE = hide_menu_bar_in_native_mode
    Parameters_CommandTab.NATIVE_THEME_MODE = native_theme_mode
    Parameters_CommandTab.NATIVE_COMPACT_PANEL_LAYOUT = compact_panel_layout
    Parameters_CommandTab.NATIVE_PANEL_DROPDOWN_MODE_ENABLED = panel_dropdown_mode_enabled
    Parameters_CommandTab.NATIVE_PANEL_DROPDOWN_PRIMARY_RECENT = panel_dropdown_primary_recent
    Parameters_CommandTab.NATIVE_PANEL_DROPDOWN_RECENT_TOOL_COUNT = panel_dropdown_recent_tool_count
    Parameters_CommandTab.NATIVE_PANEL_DROPDOWN_POPUP_COLUMNS = panel_dropdown_popup_columns
    Parameters_CommandTab.NATIVE_PANEL_DROPDOWN_POPUP_ICON_SIZE = panel_dropdown_popup_icon_size
    Parameters_CommandTab.NATIVE_DISPLAY_SCALE_PERCENT = commandtab_scale_percent
    Parameters_CommandTab.NATIVE_HEADER_SCALE_PERCENT = header_scale_percent
    Parameters_CommandTab.NATIVE_COMMANDTAB_SCALE_PERCENT = commandtab_scale_percent
    Parameters_CommandTab.NATIVE_PANEL_DROPDOWN_POPUP_SHOW_TEXT = panel_dropdown_popup_show_text
    Parameters_CommandTab.NATIVE_COMPACT_PANEL_SPACING = compact_panel_spacing
    Parameters_CommandTab.NATIVE_COMPACT_BUTTON_PADDING = compact_button_padding
    Parameters_CommandTab.NATIVE_BUTTON_BORDERS_VISIBLE = button_borders_visible
    Parameters_CommandTab.SHOW_ICON_TEXT_SMALL = show_icon_text_small
    Parameters_CommandTab.SHOW_ICON_TEXT_MEDIUM = show_icon_text_medium
    Parameters_CommandTab.SHOW_ICON_TEXT_LARGE = show_icon_text_large
    Parameters_CommandTab.NATIVE_ICON_ONLY_SIZE_SMALL = icon_only_size_small
    Parameters_CommandTab.NATIVE_ICON_ONLY_SIZE_MEDIUM = icon_only_size_medium
    Parameters_CommandTab.NATIVE_ICON_ONLY_SIZE_LARGE = icon_only_size_large
    Parameters_CommandTab.NATIVE_CUSTOM_MAIN_COLORS_ENABLED = custom_main_colors_enabled
    Parameters_CommandTab.NATIVE_CUSTOM_MAIN_BACKGROUND_COLOR = custom_main_background_color
    Parameters_CommandTab.NATIVE_CUSTOM_MAIN_TEXT_COLOR = custom_main_text_color
    Parameters_CommandTab.NATIVE_CUSTOM_RIBBON_PRIMARY_COLOR = custom_ribbon_primary_color
    Parameters_CommandTab.NATIVE_CUSTOM_RIBBON_SECONDARY_COLOR = custom_ribbon_secondary_color
    Parameters_CommandTab.NATIVE_CUSTOM_RIBBON_ACCENT_COLOR = custom_ribbon_accent_color
    Parameters_CommandTab.Settings.SetBoolSetting("RibbonAutoHide", ribbon_auto_hide)
    Parameters_CommandTab.RIBBON_AUTO_HIDE = ribbon_auto_hide
    Parameters_CommandTab.Settings.SetIntSetting("RibbonAutoHideDelayMs", ribbon_auto_hide_delay_ms)
    Parameters_CommandTab.Settings.SetBoolSetting("RibbonHoverTab", ribbon_hover_tab)
    Parameters_CommandTab.Settings.SetBoolSetting("TabClickPopupMode", tab_click_popup_mode)
    Parameters_CommandTab.TAB_CLICK_POPUP_MODE = tab_click_popup_mode
    _ct_settings.SetBool("ViewportColorsEnabled", viewport_colors_enabled)
    _ct_settings.SetString("ViewportBackgroundStyle", viewport_background_style)
    _ct_settings.SetString("ViewportBgTopColor", viewport_bg_top_color)
    _ct_settings.SetString("ViewportBgMidColor", viewport_bg_mid_color)
    _ct_settings.SetString("ViewportBgBottomColor", viewport_bg_bottom_color)
    _ct_settings.SetString("ViewportBgAccentColor", viewport_bg_accent_color)
    _ct_settings.SetBool("GridColorEnabled", grid_color_enabled)
    _ct_settings.SetString("ViewportGridColor", viewport_grid_color)
    _ct_root_settings.SetBool("ViewportColorsEnabled", viewport_colors_enabled)
    _ct_root_settings.SetString("ViewportBackgroundStyle", viewport_background_style)
    _ct_root_settings.SetString("ViewportBgTopColor", viewport_bg_top_color)
    _ct_root_settings.SetString("ViewportBgMidColor", viewport_bg_mid_color)
    _ct_root_settings.SetString("ViewportBgBottomColor", viewport_bg_bottom_color)
    _ct_root_settings.SetString("ViewportBgAccentColor", viewport_bg_accent_color)
    _ct_root_settings.SetBool("GridColorEnabled", grid_color_enabled)
    _ct_root_settings.SetString("ViewportGridColor", viewport_grid_color)
    _apply_viewport_colors(
        viewport_colors_enabled, viewport_background_style,
        viewport_bg_top_color, viewport_bg_mid_color, viewport_bg_bottom_color, viewport_bg_accent_color,
        grid_color_enabled, viewport_grid_color
    )

    App.saveParameter()

    try:
        from freecad_commandtab.native import bootstrap as NativeBootstrap

        NativeBootstrap.apply_theme_preferences()
        NativeBootstrap._ensure_sketcher_display_defaults()
        NativeBootstrap._ensure_native_workbench_ui_hidden()
    except Exception:
        pass

    _ensure_runtime_icon_cache_matches_theme()
    _invalidate_native_model_caches()
    settings_json = _native_settings_state_json()
    can_apply_in_place = (
        previous_modern_commandtab_style_enabled == modern_commandtab_style_enabled
        and previous_native_theme_mode == native_theme_mode
    )
    if can_apply_in_place and _ACTIVE_CONTROLLER is not None:
        try:
            if _ACTIVE_CONTROLLER.apply_settings(settings_json):
                return
        except Exception:
            pass
    refresh_native_commandtab(force=True, include_all_panels=False)


def _runtime_model_path() -> Path:
    return _runtime_root() / "commandtab-model.json"


def _runtime_metadata_cache_path() -> Path:
    return _runtime_root() / "metadata-cache.json"


def _persistent_variant_menu_cache_path() -> Path:
    path = Path(paths.user_cache_path("CommandTabVariantMenus.json"))
    path.parent.mkdir(parents=True, exist_ok=True)
    return path


def _runtime_structure_metadata_export_path() -> Path:
    return _runtime_root() / "structure-metadata-export.json"


def _qt_action_cache_path() -> Path:
    return _runtime_root() / "qt-action-cache.json"


def _qt_action_icons_dir() -> Path:
    icon_dir = _runtime_root() / "qt-action-icons"
    icon_dir.mkdir(parents=True, exist_ok=True)
    return icon_dir


def _persistent_icon_export_dir() -> Path:
    """Persistent icon export directory that survives FreeCAD restarts."""
    icon_dir = Path(paths.user_cache_path("icons"))
    icon_dir.mkdir(parents=True, exist_ok=True)
    return icon_dir


def _persistent_metadata_cache_path() -> Path:
    path = Path(paths.user_cache_path("CommandTabNativeMetadata.json"))
    path.parent.mkdir(parents=True, exist_ok=True)
    return path


def _qt_major_version() -> int:
    return int(str(qVersion()).split(".", 1)[0])


def _host_platform_system_name() -> str:
    return str(platform.system() or "").strip()


def _host_platform_id() -> str:
    system_name = _host_platform_system_name().lower()
    if system_name == "windows":
        return "windows"
    if system_name in ["darwin", "mac", "macos"]:
        return "macos"
    if system_name == "linux":
        return "linux"
    return system_name or "unknown"


def _platform_directory_candidates() -> list[str]:
    platform_id = _host_platform_id()
    if platform_id == "macos":
        return ["macos", "darwin"]
    if platform_id in ["windows", "linux"]:
        return [platform_id]
    return [platform_id]


def _library_filename() -> str:
    platform_id = _host_platform_id()
    if platform_id == "windows":
        return "freecad_commandtab_native_backend.dll"
    if platform_id == "macos":
        return "libfreecad_commandtab_native_backend.dylib"
    return "libfreecad_commandtab_native_backend.so"


def _binary_dir_candidates() -> list[Path]:
    qt_dir = f"qt{_qt_major_version()}"
    candidates: list[Path] = []
    for platform_dir in _platform_directory_candidates():
        paths_to_add = [
            Path(paths.addon_path("freecad_commandtab", "native", "bin", platform_dir, qt_dir)),
            Path(
                paths.addon_path(
                    "dist",
                    "FreeCAD-CommandTab",
                    "freecad_commandtab",
                    "native",
                    "bin",
                    platform_dir,
                    qt_dir,
                )
            ),
        ]
        for candidate in paths_to_add:
            if candidate not in candidates:
                candidates.append(candidate)
    return candidates


def _library_candidates() -> list[Path]:
    file_name = _library_filename()
    return [directory / file_name for directory in _binary_dir_candidates()]


def _prepare_windows_dll_search_path(library_path: Path) -> None:
    if _host_platform_id() != "windows":
        return

    library_dir = str(library_path.resolve().parent)
    add_dll_directory = getattr(os, "add_dll_directory", None)
    if callable(add_dll_directory):
        for handle in _WINDOWS_DLL_SEARCH_PATH_HANDLES:
            if getattr(handle, "_commandtab_path", "") == library_dir:
                return
        handle = add_dll_directory(library_dir)
        try:
            setattr(handle, "_commandtab_path", library_dir)
        except Exception:
            pass
        _WINDOWS_DLL_SEARCH_PATH_HANDLES.append(handle)
        return

    current_path = os.environ.get("PATH", "")
    path_parts = [part for part in current_path.split(os.pathsep) if part]
    if library_dir not in path_parts:
        os.environ["PATH"] = library_dir + os.pathsep + current_path


def _load_native_extension_module():
    # Native runtime is now loaded from the shared backend library only.
    return None


_P = ctypes.c_void_p
_S = ctypes.c_char_p
_B = ctypes.c_bool

# (symbol_name, restype, argtypes, required)
_CTYPES_SPEC = [
    # fmt: off
    ("freecad_commandtab_native_create",                       _P,    [_P, _S],                         True),
    ("freecad_commandtab_native_create_from_json",             _P,    [_P, _S],                         False),
    ("freecad_commandtab_native_destroy",                      None,  [_P],                             True),
    ("freecad_commandtab_native_reload",                       _B,    [_P, _S],                         True),
    ("freecad_commandtab_native_reload_json",                  _B,    [_P, _S],                         False),
    ("freecad_commandtab_native_reload_from_bootstrap",        _B,    [_P, _S, _S, _S, _B, _S, _S],    False),
    ("freecad_commandtab_native_open_customization_dialog",    _B,    [_P, _S, _S],                     False),
    ("freecad_commandtab_native_open_settings_dialog",         _B,    [_P, _S],                         False),
    ("freecad_commandtab_native_apply_settings_json",          _B,    [_P, _S],                         False),
    ("freecad_commandtab_native_export_qt_action_cache",       _B,    [_P, _S, _S],                     False),
    ("freecad_commandtab_native_export_qt_action_cache_to_memory", _P, [_P, _S],                        False),
    ("freecad_commandtab_native_export_structure_metadata_cache", _B, [_P, _S, _S, _S, _S, _B, _B],    False),
    ("freecad_commandtab_native_build_persistent_metadata_cache", _B, [_P, _S, _S, _S, _B, _B, _S, _S, _S], False),
    ("freecad_commandtab_native_apply_customization_to_structure", _B, [_S, _S],                        False),
    ("freecad_commandtab_native_export_structure_copy",        _B,    [_S, _S],                         False),
    ("freecad_commandtab_native_import_structure_copy",        _B,    [_S, _S],                         False),
    ("freecad_commandtab_native_reset_structure_copy",         _B,    [_S, _S],                         False),
    ("freecad_commandtab_native_restore_structure_from_backup", _B,   [_S, _S],                         False),
    ("freecad_commandtab_native_set_active_workbench",         _B,    [_P, _S],                         False),
    ("freecad_commandtab_native_upsert_workbench_json",        _B,    [_P, _S, _B],                     False),
    ("freecad_commandtab_native_upsert_workbench_from_bootstrap", _B, [_P, _S, _S, _S, _S, _S, _B],   False),
    ("freecad_commandtab_native_enable_ui_hide_controller",    _B,    [_P, _B],                         False),
    ("freecad_commandtab_native_set_command_callback",         None,  [_P, _P, _P],                     True),
    ("freecad_commandtab_native_platform_id",                  _S,    [],                               False),
    ("freecad_commandtab_native_is_supported_platform",        _B,    [],                               False),
    ("freecad_commandtab_native_last_error",                   _S,    [],                               True),
    ("freecad_commandtab_native_build_bootstrap_json",         _P,    [_S, _S, _S, _B, _S, _S],        False),
    ("freecad_commandtab_native_build_workbench_bootstrap_json", _P,  [_S, _S, _S, _S, _S],            False),
    ("freecad_commandtab_native_build_model_json",             _P,    [_S, _S, _S, _B, _S, _S],        False),
    ("freecad_commandtab_native_build_workbench_json",         _P,    [_S, _S, _S, _S],                False),
    ("freecad_commandtab_native_free_string",                  None,  [_P],                             False),
    ("freecad_commandtab_native_free_buffer",                  None,  [_P],                             False),
    # fmt: on
]


def _apply_ctypes_spec(library: ctypes.CDLL) -> None:
    for name, restype, argtypes, required in _CTYPES_SPEC:
        fn = getattr(library, name, None)
        if fn is None:
            if required:
                raise AttributeError(f"Required symbol '{name}' not found in native library")
            continue
        fn.restype = restype
        fn.argtypes = argtypes


def _read_native_library_platform(library: ctypes.CDLL) -> tuple[str, bool | None]:
    platform_id = ""
    platform_supported = None

    platform_id_fn = getattr(library, "freecad_commandtab_native_platform_id", None)
    if callable(platform_id_fn):
        try:
            value = platform_id_fn()
            if value:
                platform_id = str(value.decode("utf-8", errors="replace")).strip().lower()
        except Exception:
            platform_id = ""

    supported_fn = getattr(library, "freecad_commandtab_native_is_supported_platform", None)
    if callable(supported_fn):
        try:
            platform_supported = bool(supported_fn())
        except Exception:
            platform_supported = None

    return platform_id, platform_supported


def _load_native_library() -> ctypes.CDLL:
    global _NATIVE_RUNTIME_KIND
    global _NATIVE_RUNTIME_PATH
    global _NATIVE_RUNTIME_LIBRARY_PLATFORM
    global _NATIVE_RUNTIME_LIBRARY_PLATFORM_SUPPORTED
    with StartupTrace.span("bridge.load_native_library"):
        _set_native_runtime_diagnostics("loading_shared_library", None)
        _NATIVE_RUNTIME_LIBRARY_PLATFORM = ""
        _NATIVE_RUNTIME_LIBRARY_PLATFORM_SUPPORTED = None
        for candidate in _library_candidates():
            if candidate.exists():
                _prepare_windows_dll_search_path(candidate)
                library = ctypes.CDLL(str(candidate))
                _apply_ctypes_spec(library)
                (
                    _NATIVE_RUNTIME_LIBRARY_PLATFORM,
                    _NATIVE_RUNTIME_LIBRARY_PLATFORM_SUPPORTED,
                ) = _read_native_library_platform(library)
                _NATIVE_RUNTIME_KIND = "shared_library"
                _NATIVE_RUNTIME_PATH = str(candidate)
                _set_native_runtime_diagnostics("shared_library_loaded", "")
                StartupTrace.mark(
                    "bridge.native_library_loaded",
                    runtimeKind=_NATIVE_RUNTIME_KIND,
                    runtimePath=_NATIVE_RUNTIME_PATH,
                    runtimeLibraryPlatform=_NATIVE_RUNTIME_LIBRARY_PLATFORM,
                    runtimeLibraryPlatformSupported=_NATIVE_RUNTIME_LIBRARY_PLATFORM_SUPPORTED,
                )
                return library

        candidate_list = ", ".join(str(path) for path in _library_candidates())
        _set_native_runtime_diagnostics(
            "shared_library_missing",
            (
                "Native commandtab library not found for Qt "
                f"{_qt_major_version()}. Expected one of: {candidate_list}"
            ),
        )
        raise FileNotFoundError(
            f"Native commandtab library not found for Qt {_qt_major_version()}. Expected one of: {candidate_list}"
        )


def _library_error(library: ctypes.CDLL) -> str:
    try:
        value = library.freecad_commandtab_native_last_error()
        if value:
            return value.decode("utf-8", errors="replace")
    except Exception:
        pass
    return "Unknown native commandtab error"


def _resolve_cpp_pointer(widget) -> int:
    for module_name in ["shiboken6", "shiboken2", "shiboken"]:
        try:
            module = __import__(module_name)
            pointer = module.getCppPointer(widget)[0]
            return int(pointer)
        except Exception:
            continue
    raise RuntimeError("Unable to resolve the Qt C++ pointer for the FreeCAD main window")


def _current_workbench_name() -> str:
    available_workbenches = _available_workbenches()
    if len(available_workbenches) == 0:
        try:
            return str(Gui.activeWorkbench().__class__.__name__)
        except Exception:
            pass
        try:
            return str(Gui.activeWorkbench().name())
        except Exception:
            return ""

    active_workbench = None
    try:
        active_workbench = Gui.activeWorkbench()
    except Exception:
        active_workbench = None

    if active_workbench is not None:
        for workbench_name, workbench in available_workbenches.items():
            try:
                if workbench is active_workbench:
                    return workbench_name
            except Exception:
                continue

        try:
            active_class_name = str(active_workbench.__class__.__name__ or "").strip()
        except Exception:
            active_class_name = ""

        if active_class_name in available_workbenches:
            return active_class_name
        if active_class_name != "":
            for workbench_name, workbench in available_workbenches.items():
                try:
                    if str(workbench.__class__.__name__ or "").strip() == active_class_name:
                        return workbench_name
                except Exception:
                    continue

        try:
            active_title = str(getattr(active_workbench, "MenuText", "") or "").replace("&", "").strip()
        except Exception:
            active_title = ""
        if active_title != "":
            for workbench_name, workbench in available_workbenches.items():
                try:
                    workbench_title = str(getattr(workbench, "MenuText", "") or "").replace("&", "").strip()
                except Exception:
                    workbench_title = ""
                if workbench_title == active_title and workbench_title != "":
                    return workbench_name

        try:
            active_name = str(active_workbench.name() or "").strip()
        except Exception:
            active_name = ""
        if active_name in available_workbenches:
            return active_name
        if active_name != "":
            for workbench_name in available_workbenches:
                if workbench_name.strip().lower() == active_name.lower():
                    return workbench_name

    try:
        return str(Gui.activeWorkbench().__class__.__name__)
    except Exception:
        pass

    try:
        return str(Gui.activeWorkbench().name())
    except Exception:
        return ""


def _available_workbenches() -> dict[str, object]:
    global _AVAILABLE_WORKBENCHES_CACHE_VALID
    global _AVAILABLE_WORKBENCHES_CACHE_TS
    global _AVAILABLE_WORKBENCHES_CACHE

    now = time.monotonic()
    if (
        _AVAILABLE_WORKBENCHES_CACHE_VALID
        and (now - float(_AVAILABLE_WORKBENCHES_CACHE_TS)) <= _AVAILABLE_WORKBENCHES_CACHE_TTL_S
    ):
        return _AVAILABLE_WORKBENCHES_CACHE

    try:
        workbenches = Gui.listWorkbenches()
    except Exception:
        return {}

    if isinstance(workbenches, dict):
        normalized = {str(name): value for name, value in workbenches.items()}
        _AVAILABLE_WORKBENCHES_CACHE = normalized
        _AVAILABLE_WORKBENCHES_CACHE_VALID = True
        _AVAILABLE_WORKBENCHES_CACHE_TS = now
        return normalized

    normalized: dict[str, object] = {}
    try:
        for name, value in dict(workbenches).items():
            normalized[str(name)] = value
    except Exception:
        return {}
    _AVAILABLE_WORKBENCHES_CACHE = normalized
    _AVAILABLE_WORKBENCHES_CACHE_VALID = True
    _AVAILABLE_WORKBENCHES_CACHE_TS = now
    return normalized


def _is_available_workbench(workbench_name: str) -> bool:
    workbench_name = str(workbench_name or "").strip()
    if workbench_name == "":
        return False
    normalized = workbench_name.lower()
    if normalized in {"noneworkbench", "<none>", "none"}:
        return False
    return workbench_name in _available_workbenches()


def _process_gui_events(duration_ms: int = 120) -> None:
    try:
        application = QApplication.instance()
    except Exception:
        application = None
    if application is None:
        return

    deadline = time.monotonic() + max(0, int(duration_ms)) / 1000.0
    while time.monotonic() < deadline:
        try:
            application.processEvents()
        except Exception:
            return
        time.sleep(0.01)


def _ensure_workbenches_loaded(workbench_names: set[str]) -> None:
    global _WORKBENCH_PRELOAD_IN_PROGRESS
    global _AVAILABLE_WORKBENCHES_CACHE_VALID
    global _AVAILABLE_WORKBENCHES_CACHE_TS

    if _WORKBENCH_PRELOAD_ENABLED is False:
        return

    available_workbenches = _available_workbenches()
    if len(available_workbenches) == 0:
        return

    ordered_names: list[str] = []
    for workbench_name in sorted(str(name or "").strip() for name in workbench_names):
        if workbench_name == "" or workbench_name not in available_workbenches:
            continue
        if workbench_name not in ordered_names:
            ordered_names.append(workbench_name)
    if len(ordered_names) == 0:
        return

    previously_active = _current_workbench_name()
    _WORKBENCH_PRELOAD_IN_PROGRESS = True
    try:
        for workbench_name in ordered_names:
            try:
                Gui.activateWorkbench(workbench_name)
                _process_gui_events(140)
            except Exception:
                continue
    finally:
        if previously_active in available_workbenches:
            try:
                Gui.activateWorkbench(previously_active)
                _process_gui_events(140)
            except Exception:
                pass
        _AVAILABLE_WORKBENCHES_CACHE_VALID = False
        _AVAILABLE_WORKBENCHES_CACHE_TS = 0.0
        _WORKBENCH_PRELOAD_IN_PROGRESS = False


def _filter_native_payload_workbenches(payload: dict) -> dict:
    if isinstance(payload, dict) is False:
        return {}

    available_names = set(_available_workbenches().keys())
    if len(available_names) == 0:
        return payload

    filtered_workbenches: list[dict] = []
    seen_workbench_ids: set[str] = set()
    for workbench in _coerce_list(payload.get("workbenches", [])):
        workbench_object = _coerce_dict(workbench)
        workbench_id = str(workbench_object.get("id") or "").strip()
        normalized_workbench_id = workbench_id.lower()
        if normalized_workbench_id in {"noneworkbench", "<none>", "none"}:
            continue
        if workbench_id == "" or workbench_id not in available_names:
            continue
        if workbench_id in seen_workbench_ids:
            continue
        filtered_workbenches.append(workbench_object)
        seen_workbench_ids.add(workbench_id)
    payload["workbenches"] = filtered_workbenches

    active_workbench = str(payload.get("activeWorkbenchId") or "").strip()
    if active_workbench not in seen_workbench_ids:
        current_workbench = _current_workbench_name()
        if current_workbench in seen_workbench_ids:
            payload["activeWorkbenchId"] = current_workbench
        elif len(filtered_workbenches) > 0:
            payload["activeWorkbenchId"] = str(filtered_workbenches[0].get("id") or "")
        else:
            payload["activeWorkbenchId"] = ""

    return payload


def _workbench_title(workbench_name: str) -> str:
    cached_value = _WORKBENCH_TITLE_CACHE.get(workbench_name)
    if cached_value is not None:
        return cached_value

    workbench = None
    try:
        workbench = Gui.getWorkbench(workbench_name)
    except Exception:
        workbench = None
    if workbench is None:
        workbench = _available_workbenches().get(workbench_name)
    if workbench is None:
        return workbench_name

    for attribute in ["MenuText", "menuText", "Label", "label"]:
        try:
            value = getattr(workbench, attribute)
            if callable(value):
                value = value()
            value = str(value).strip()
            if value != "":
                break
        except Exception:
            pass
    else:
        value = workbench_name

    cleaned = str(value).replace("&", "").strip()
    if cleaned in ["", workbench_name] or cleaned.endswith("Workbench"):
        cleaned = re.sub(r"Workbench$", "", cleaned).strip()
        cleaned = re.sub(r"(?<!^)([A-Z])", r" \1", cleaned).strip()
        cleaned = re.sub(r"\s+", " ", cleaned).strip()
    if cleaned == "":
        cleaned = workbench_name
    _WORKBENCH_TITLE_CACHE[workbench_name] = cleaned
    return cleaned


def _is_missing_or_stale_workbench_title(workbench_name: str, title_value: str) -> bool:
    normalized_workbench_name = str(workbench_name or "").strip()
    title = str(title_value or "").strip()
    if title == "":
        return True
    if normalized_workbench_name != "" and title == normalized_workbench_name:
        return True

    normalized_title = title.lower()
    if normalized_title in {"<none>", "none", "noneworkbench"}:
        return True
    return False


def _is_missing_or_stale_workbench_icon(workbench_name: str, icon_value: str) -> bool:
    normalized_workbench_name = str(workbench_name or "").strip()
    normalized_icon = str(icon_value or "").strip()
    if normalized_icon == "":
        return True

    lowered = normalized_icon.lower()
    if lowered in {"<none>", "none", "noneworkbench"}:
        return True
    if lowered.endswith("/noneworkbench.svg") or lowered.endswith("/noneworkbench.png"):
        return True

    normalized_path = normalized_icon.replace("\\", "/").lower()
    if "/qt-action-icons/" in normalized_path:
        return True

    expected_tokens: set[str] = set()

    def _add_token(token: str) -> None:
        token = str(token or "").strip().lower()
        if token != "":
            expected_tokens.add(token)

    wb_base = normalized_workbench_name
    if wb_base.endswith("Workbench") and len(wb_base) > len("Workbench"):
        wb_base = wb_base[: -len("Workbench")]

    _add_token(normalized_workbench_name)
    _add_token(wb_base)

    wb_title = str(_workbench_title(normalized_workbench_name) or "").replace("&", "").strip()
    _add_token(wb_title)
    _add_token(wb_title.replace(" ", ""))
    _add_token(wb_title.replace(" ", "_"))
    _add_token(wb_title.replace(" ", "-"))
    _add_token(wb_title.replace("-", ""))
    _add_token(wb_title.replace("_", ""))

    alias_tokens: dict[str, list[str]] = {
        "surfaceworkbench": ["surface_workbench"],
        "startworkbench": ["start", "startcommandicon", "preferences-start"],
        "inspectionworkbench": ["inspection"],
        "openscadworkbench": ["openscad", "preferences-openscad"],
        "pointsworkbench": ["points", "point"],
        "reverseengineeringworkbench": ["reverseengineering", "reverse_engineering"],
    }
    for token in alias_tokens.get(normalized_workbench_name.lower(), []):
        _add_token(token)

    icon_name = Path(normalized_icon).stem.lower()
    icon_name_compact = icon_name.replace("-", "").replace("_", "").replace(" ", "")
    if icon_name_compact == "":
        icon_name_compact = normalized_icon.lower()

    if len(expected_tokens) > 0:
        token_match = False
        for token in expected_tokens:
            token_compact = token.replace("-", "").replace("_", "").replace(" ", "")
            if token_compact == "":
                continue
            if token_compact in icon_name_compact:
                token_match = True
                break
        if token_match is False:
            return True

    if _is_stable_icon_reference(normalized_icon) is False:
        return True
    return False


def _bundled_workbench_icon_stems(workbench_name: str) -> list[str]:
    normalized = str(workbench_name or "").strip().lower()
    aliases = {
        "archworkbench": ["BIMWorkbench", "Arch_Building"],
        "camworkbench": ["CAM_Job", "CAM_Toolpath", "PathWorkbench"],
        "pathworkbench": ["CAM_Job", "CAM_Toolpath", "PathWorkbench"],
        "startworkbench": ["StartCommandIcon", "preferences-start"],
        "techdrawworkbench": ["TechDraw_PageDefault", "TechDraw_View"],
        "measureworkbench": ["Measurement-Distance", "Measurement-Group"],
        "webworkbench": ["internet-web-browser", "help-browser"],
    }
    return aliases.get(normalized, [])


def _runtime_workbench_icons_dir() -> Path:
    icon_dir = _runtime_root() / "workbench-icons"
    icon_dir.mkdir(parents=True, exist_ok=True)
    return icon_dir


def _render_svg_pixmap(svg_payload: bytes, requested_size: QSize) -> QPixmap:
    try:
        from PySide.QtSvg import QSvgRenderer
    except Exception:
        return QPixmap()

    try:
        renderer = QSvgRenderer(QByteArray(svg_payload))
    except Exception:
        renderer = QSvgRenderer()
        try:
            renderer.load(QByteArray(svg_payload))
        except Exception:
            return QPixmap()

    if renderer.isValid() is False:
        return QPixmap()

    render_size = QSize(requested_size)
    if render_size.isValid() is False or render_size.isEmpty():
        render_size = renderer.defaultSize()
    if render_size.isValid() is False or render_size.isEmpty():
        render_size = QSize(48, 48)

    pixmap = QPixmap(render_size)
    pixmap.fill(Qt.transparent)
    painter = QPainter(pixmap)
    try:
        renderer.render(painter)
    finally:
        painter.end()
    return pixmap


def _inline_icon_pixmap(raw_icon_value, requested_size: QSize) -> QPixmap:
    if raw_icon_value in [None, ""]:
        return QPixmap()

    if isinstance(raw_icon_value, (list, tuple)):
        xpm_lines = [str(line) for line in raw_icon_value if str(line).strip() != ""]
        if len(xpm_lines) > 0:
            try:
                pixmap = QPixmap(xpm_lines)
            except Exception:
                pixmap = QPixmap()
            if pixmap.isNull() is False:
                return pixmap

    if isinstance(raw_icon_value, (bytes, bytearray)):
        raw_bytes = bytes(raw_icon_value)
        if b"<svg" in raw_bytes or b"<?xml" in raw_bytes:
            pixmap = _render_svg_pixmap(raw_bytes, requested_size)
            if pixmap.isNull() is False:
                return pixmap
        pixmap = QPixmap()
        try:
            pixmap.loadFromData(raw_bytes)
        except Exception:
            pixmap = QPixmap()
        return pixmap

    raw_text = str(raw_icon_value or "").strip()
    if raw_text == "":
        return QPixmap()

    if raw_text.startswith("data:image/") and "," in raw_text:
        header, payload = raw_text.split(",", 1)
        image_bytes = b""
        if ";base64" in header:
            try:
                image_bytes = base64.b64decode(payload, validate=False)
            except Exception:
                image_bytes = b""
        else:
            image_bytes = payload.encode("utf-8", errors="ignore")

        if len(image_bytes) > 0:
            if "svg" in header.lower() or b"<svg" in image_bytes or b"<?xml" in image_bytes:
                pixmap = _render_svg_pixmap(image_bytes, requested_size)
                if pixmap.isNull() is False:
                    return pixmap

            pixmap = QPixmap()
            try:
                pixmap.loadFromData(image_bytes)
            except Exception:
                pixmap = QPixmap()
            if pixmap.isNull() is False:
                return pixmap

    if "<svg" in raw_text or raw_text.startswith("<?xml"):
        pixmap = _render_svg_pixmap(raw_text.encode("utf-8"), requested_size)
        if pixmap.isNull() is False:
            return pixmap

    if "XPM" in raw_text and "\n" in raw_text:
        xpm_lines: list[str] = []
        for line in raw_text.splitlines():
            stripped = line.strip()
            if stripped in ["", "{", "};"]:
                continue
            if stripped.startswith("/*") or stripped.startswith("static "):
                continue
            stripped = stripped.rstrip(",")
            if stripped.startswith('"') and stripped.endswith('"'):
                stripped = stripped[1:-1]
            if stripped != "":
                xpm_lines.append(stripped)

        if len(xpm_lines) > 0:
            try:
                pixmap = QPixmap(xpm_lines)
            except Exception:
                pixmap = QPixmap()
            if pixmap.isNull() is False:
                return pixmap

    pixmap = QPixmap()
    try:
        pixmap.loadFromData(raw_text.encode("utf-8", errors="ignore"))
    except Exception:
        pixmap = QPixmap()
    return pixmap


def _pixmap_from_icon_source_path(icon_path: Path, requested_size: QSize) -> QPixmap:
    try:
        resolved_path = icon_path.expanduser().resolve()
    except Exception:
        resolved_path = icon_path

    try:
        suffix = resolved_path.suffix.lower()
    except Exception:
        suffix = ""

    if suffix in [".svg", ".svgz"]:
        try:
            return _render_svg_pixmap(resolved_path.read_bytes(), requested_size)
        except Exception:
            return QPixmap()

    pixmap = QPixmap()
    try:
        pixmap.load(str(resolved_path))
    except Exception:
        pixmap = QPixmap()
    return pixmap


def _workbench_icon_export_path(workbench_name: str, persistent: bool = False) -> Path:
    safe_name = (
        str(workbench_name or "")
        .replace("/", "_")
        .replace("\\", "_")
        .replace(" ", "_")
    )
    if safe_name == "":
        safe_name = "workbench"
    if persistent:
        return _persistent_icon_export_dir() / f"workbench_{safe_name}.png"
    return _runtime_workbench_icons_dir() / f"{safe_name}.png"


def _save_workbench_icon_pixmap(
    workbench_name: str, pixmap: QPixmap, persistent: bool = False
) -> str:
    if pixmap is None or pixmap.isNull():
        if not persistent:
            _WORKBENCH_ICON_CACHE[workbench_name] = ""
        return ""

    output_path = _workbench_icon_export_path(workbench_name, persistent=persistent)
    try:
        pixmap.save(str(output_path), "PNG")
    except Exception:
        if not persistent:
            _WORKBENCH_ICON_CACHE[workbench_name] = ""
        return ""

    resolved = str(output_path)
    if not persistent:
        _WORKBENCH_ICON_CACHE[workbench_name] = resolved
    return resolved


def _workbench_icon_path(workbench_name: str, persistent: bool = False) -> str:
    if not persistent:
        cached_value = _WORKBENCH_ICON_CACHE.get(workbench_name)
        if cached_value is not None:
            return cached_value
    else:
        persistent_path = _workbench_icon_export_path(workbench_name, persistent=True)
        if persistent_path.exists():
            return str(persistent_path)

    raw_icon_value = None
    workbench = None
    try:
        workbench = Gui.getWorkbench(workbench_name)
    except Exception:
        workbench = None
    if workbench is None:
        workbench = _available_workbenches().get(workbench_name)

    if workbench is not None:
        for attribute in ["Icon", "icon", "Pixmap", "pixmap"]:
            try:
                value = getattr(workbench, attribute)
                if callable(value):
                    value = value()
                if value not in [None, ""]:
                    raw_icon_value = value
                    break
            except Exception:
                continue

    def _candidate_tokens() -> list[str]:
        candidates: list[str] = []

        def _append(token: str) -> None:
            token = str(token or "").strip()
            if token != "" and token not in candidates:
                candidates.append(token)

        def _append_with_variants(token: str) -> None:
            token = str(token or "").strip()
            if token == "":
                return
            _append(token)
            base = Path(token).stem.strip()
            if base == "":
                return
            _append(f":/icons/{base}.svg")
            _append(f":/icons/{base}.png")
            _append(f"icons:{base}")
            _append(f":/icons/Workbench_{base}.svg")
            _append(f":/icons/Workbench_{base}.png")
            _append(f"icons:Workbench_{base}")

        def _append_compound_variants(token: str) -> None:
            token = str(token or "").strip()
            if token == "":
                return
            compact = token.replace(" ", "")
            underscored = compact.replace("-", "_")
            dashed = compact.replace("_", "-")
            spaced = re.sub(r"(?<!^)([A-Z])", r" \1", compact).strip()
            lowercase = compact.lower()
            parts = [compact, underscored, dashed, spaced, lowercase]
            for part in parts:
                part = str(part or "").strip()
                if part == "":
                    continue
                _append_with_variants(part)

        _append_with_variants(workbench_name)
        if workbench_name.endswith("Workbench") and len(workbench_name) > len("Workbench"):
            _append_with_variants(workbench_name[: -len("Workbench")])
            _append_compound_variants(workbench_name[: -len("Workbench")])
        title = str(_workbench_title(workbench_name) or "").replace("&", "").strip()
        _append_with_variants(title)
        _append_with_variants(title.replace(" ", ""))
        _append_with_variants(title.replace(" ", "_"))
        _append_with_variants(title.replace(" ", "-"))
        _append_compound_variants(title)
        _append_compound_variants(workbench_name)
        normalized_workbench_name = str(workbench_name or "").strip().lower()
        alias_tokens = {
            "inspectionworkbench": ["inspection"],
            "openscadworkbench": ["openscad", "preferences-openscad"],
            "pointsworkbench": ["points"],
            "reverseengineeringworkbench": ["reverseengineering", "reverse_engineering"],
        }
        for alias in alias_tokens.get(normalized_workbench_name, []):
            _append_with_variants(alias)
        for alias in _bundled_workbench_icon_stems(workbench_name):
            _append_with_variants(alias)
        return candidates

    def _candidate_icon_files() -> list[Path]:
        candidates: list[Path] = []
        seen: set[str] = set()

        def _append(path: Path) -> None:
            try:
                key = str(path.resolve())
            except Exception:
                key = str(path)
            if key in seen:
                return
            seen.add(key)
            candidates.append(path)

        wb_name = str(workbench_name or "").strip()
        wb_base = wb_name
        if wb_base.endswith("Workbench"):
            wb_base = wb_base[: -len("Workbench")]
        wb_base = wb_base.strip()

        title = str(_workbench_title(workbench_name) or "").replace("&", "").strip()
        title_compact = title.replace(" ", "")
        title_underscore = title.replace(" ", "_")
        title_dash = title.replace(" ", "-")

        module_names: list[str] = []
        for module_name in [wb_base, wb_name, title_compact, title_underscore, title_dash]:
            module_name = str(module_name or "").strip()
            if module_name != "" and module_name not in module_names:
                module_names.append(module_name)

        icon_stems: list[str] = []
        for stem in [wb_name, wb_base, title_compact, title_underscore, title_dash]:
            stem = str(stem or "").strip()
            if stem != "" and stem not in icon_stems:
                icon_stems.append(stem)
        for stem in _bundled_workbench_icon_stems(workbench_name):
            stem = str(stem or "").strip()
            if stem != "" and stem not in icon_stems:
                icon_stems.append(stem)

        resource_dir = Path()
        try:
            resource_dir = Path(str(App.getResourceDir() or "")).expanduser()
        except Exception:
            resource_dir = Path()

        mod_root = resource_dir / "Mod"
        if mod_root.exists():
            for module_name in module_names:
                icon_dir = mod_root / module_name / "Resources" / "icons"
                if not icon_dir.exists():
                    continue
                for stem in icon_stems:
                    for suffix in [".svg", ".png", ".xpm", ".svgz"]:
                        _append(icon_dir / f"{stem}{suffix}")
                if wb_base != "" and wb_base not in icon_stems:
                    for suffix in [".svg", ".png", ".xpm", ".svgz"]:
                        _append(icon_dir / f"{wb_base}{suffix}")
                if wb_name != "" and wb_name not in icon_stems:
                    for suffix in [".svg", ".png", ".xpm", ".svgz"]:
                        _append(icon_dir / f"{wb_name}{suffix}")

        bundled_icon_dir = (
            Path(paths.addon_path("Export", "IconThemes", "FreeCAD-CommandTab-Modern", "scalable"))
        )
        if bundled_icon_dir.exists():
            for stem in icon_stems:
                for suffix in [".svg", ".png", ".xpm", ".svgz"]:
                    _append(bundled_icon_dir / f"{stem}{suffix}")

        return candidates

    def _resolve_icon(tokens: list[str]) -> QIcon:
        allow_gui_icon_fallback = str(
            os.environ.get("FREECAD_COMMANDTAB_ENABLE_GUI_ICON_FALLBACK", "")
        ).strip().lower() in ["1", "true", "yes", "on"]
        for token in tokens:
            icon = QIcon(token)
            if icon.isNull() and allow_gui_icon_fallback:
                try:
                    icon = Gui.getIcon(token)
                except Exception:
                    icon = QIcon()
            if icon.isNull() and QIcon.hasThemeIcon(token):
                icon = QIcon.fromTheme(token)
            if icon.isNull() is False:
                return icon
        return QIcon()

    # Prefer explicit module icon files first (Mod/<Workbench>/Resources/icons).
    # This avoids ambiguous token resolution that can pick unrelated command icons.
    for file_candidate in _candidate_icon_files():
        try:
            if file_candidate.exists():
                pixmap = _pixmap_from_icon_source_path(file_candidate, QSize(48, 48))
                if pixmap.isNull() is False:
                    return _save_workbench_icon_pixmap(
                        workbench_name, pixmap, persistent=persistent
                    )
        except Exception:
            continue

    if raw_icon_value in [None, ""]:
        icon = _resolve_icon(_candidate_tokens())
        if icon.isNull():
            if not persistent:
                _WORKBENCH_ICON_CACHE[workbench_name] = ""
            return ""
    else:
        try:
            if isinstance(raw_icon_value, str):
                direct_path = Path(raw_icon_value)
                if direct_path.exists():
                    direct_pixmap = _pixmap_from_icon_source_path(
                        direct_path, QSize(48, 48)
                    )
                    if direct_pixmap.isNull() is False:
                        return _save_workbench_icon_pixmap(
                            workbench_name, direct_pixmap, persistent=persistent
                        )
                    resolved = str(direct_path)
                    if not persistent:
                        _WORKBENCH_ICON_CACHE[workbench_name] = resolved
                    return resolved
        except Exception:
            pass

        icon = QIcon()
        if isinstance(raw_icon_value, QIcon):
            icon = raw_icon_value
        else:
            pixmap = _inline_icon_pixmap(raw_icon_value, QSize(48, 48))
            if pixmap.isNull() is False:
                icon = QIcon(pixmap)
        if icon.isNull():
            try:
                icon = QIcon(raw_icon_value)
            except Exception:
                try:
                    icon = QIcon(str(raw_icon_value))
                except Exception:
                    icon = QIcon()
        if icon.isNull():
            icon = _resolve_icon(_candidate_tokens())
        if icon.isNull():
            if not persistent:
                _WORKBENCH_ICON_CACHE[workbench_name] = ""
            return ""

    pixmap = icon.pixmap(QSize(48, 48))
    if pixmap is None or pixmap.isNull():
        pixmap = icon.pixmap(QSize(32, 32))
    return _save_workbench_icon_pixmap(
        workbench_name, pixmap, persistent=persistent
    )


def _qt_action_metadata_cache() -> dict[str, dict]:
    theme_signature = "|".join(_native_theme_signature())
    active_workbench = _current_workbench_name()
    cache_key = (theme_signature, active_workbench)
    cached_value = _QT_ACTION_CACHE.get(cache_key)
    if cached_value is not None:
        return cached_value

    with StartupTrace.span(
        "bridge.qt_action_metadata_cache", activeWorkbench=active_workbench
    ):
        _ensure_runtime_icon_cache_matches_theme()
        output_path = _qt_action_cache_path()
        icon_dir = _qt_action_icons_dir()

        try:
            main_window = Gui.getMainWindow()
            main_window_ptr = _resolve_cpp_pointer(main_window)
        except Exception:
            _QT_ACTION_CACHE_FAILURES[cache_key] = _QT_ACTION_CACHE_FAILURES.get(cache_key, 0) + 1
            return {}

        exported = False
        payload = None
        native_module = _load_native_extension_module()
        if native_module is not None:
            export_qt_action_cache = getattr(native_module, "export_qt_action_cache", None)
            if callable(export_qt_action_cache):
                try:
                    export_qt_action_cache(main_window_ptr, str(output_path), str(icon_dir))
                    exported = True
                except Exception:
                    exported = False

        if exported is False:
            try:
                library = _load_native_library()
            except Exception:
                library = None
            if (
                library is not None
                and _is_cpp_qaction_memcache_enabled() is True
                and hasattr(library, "freecad_commandtab_native_export_qt_action_cache_to_memory")
            ):
                raw_payload = None
                try:
                    raw_payload = library.freecad_commandtab_native_export_qt_action_cache_to_memory(
                        main_window_ptr,
                        str(icon_dir).encode("utf-8"),
                    )
                    if raw_payload:
                        payload_bytes = ctypes.cast(raw_payload, ctypes.c_char_p).value
                        if payload_bytes:
                            decoded_payload = payload_bytes.decode("utf-8")
                            parsed_payload = json.loads(decoded_payload)
                            if isinstance(parsed_payload, dict):
                                payload = parsed_payload
                                exported = True
                except Exception:
                    payload = None
                    exported = False
                finally:
                    if raw_payload:
                        try:
                            if hasattr(library, "freecad_commandtab_native_free_buffer"):
                                library.freecad_commandtab_native_free_buffer(raw_payload)
                            elif hasattr(library, "freecad_commandtab_native_free_string"):
                                library.freecad_commandtab_native_free_string(raw_payload)
                        except Exception:
                            pass

            if (
                exported is False
                and library is not None
                and hasattr(library, "freecad_commandtab_native_export_qt_action_cache")
            ):
                exported = bool(
                    library.freecad_commandtab_native_export_qt_action_cache(
                        main_window_ptr,
                        str(output_path).encode("utf-8"),
                        str(icon_dir).encode("utf-8"),
                    )
                )

        if exported is False:
            _QT_ACTION_CACHE_FAILURES[cache_key] = _QT_ACTION_CACHE_FAILURES.get(cache_key, 0) + 1
            return {}

        if payload is None:
            if output_path.exists() is False:
                _QT_ACTION_CACHE_FAILURES[cache_key] = _QT_ACTION_CACHE_FAILURES.get(cache_key, 0) + 1
                return {}
            try:
                payload = json.loads(output_path.read_text(encoding="utf-8"))
            except Exception:
                payload = {}
        commands = _coerce_dict(payload.get("commands", {})) if isinstance(payload, dict) else {}

        normalized_commands: dict[str, dict] = {}
        for command_name, command_data in commands.items():
            command_name = str(command_name or "").strip()
            if command_name == "" or not isinstance(command_data, dict):
                continue
            normalized_commands[command_name] = {
                "text": str(command_data.get("text") or ""),
                "toolTip": str(command_data.get("toolTip") or ""),
                "iconPath": str(command_data.get("iconPath") or ""),
            }

        if normalized_commands:
            _QT_ACTION_CACHE[cache_key] = normalized_commands
            _QT_ACTION_CACHE_FAILURES.pop(cache_key, None)
            StartupTrace.mark(
                "bridge.qt_action_metadata_cache_ready",
                activeWorkbench=active_workbench,
                commandCount=len(normalized_commands),
            )
        else:
            failure_count = _QT_ACTION_CACHE_FAILURES.get(cache_key, 0) + 1
            _QT_ACTION_CACHE_FAILURES[cache_key] = failure_count
            if failure_count >= 3:
                _QT_ACTION_CACHE[cache_key] = {}
        return normalized_commands


def _qt_action_command_metadata(command_name: str) -> dict:
    if str(command_name or "").strip() == "":
        return {}

    metadata_cache = _qt_action_metadata_cache()
    for candidate in NativeCommandMetadata.command_icon_candidates(command_name):
        metadata = metadata_cache.get(candidate)
        if isinstance(metadata, dict):
            return dict(metadata)
    return {}


def _qt_action_command_payload(command_name: str, command_data: dict | None = None) -> dict | None:
    qt_action_metadata = _qt_action_command_metadata(command_name)
    if not isinstance(qt_action_metadata, dict):
        return None

    text = str(qt_action_metadata.get("text") or "").replace("&", "").strip()
    if _looks_like_technical_command_text(text, command_name):
        text = ""
    icon_path = str(qt_action_metadata.get("iconPath") or "").strip()
    if text == "" and icon_path == "":
        return None

    icon_hint = ""
    if isinstance(command_data, dict):
        icon_hint = str(command_data.get("icon") or "").strip()
    if icon_hint.startswith(":/"):
        return None
    if icon_hint != "" and Path(icon_hint).exists():
        return None

    return {
        "text": text,
        "iconPath": icon_path,
    }


def _runtime_icon_path(command_name: str, icon_hint: str = "", theme_signature: str = "") -> Path:
    digest = hashlib.sha1(
        (
            f"{command_name}|{icon_hint}|{theme_signature}"
            f"|metadata-v{_NATIVE_METADATA_CACHE_VERSION}|native-icon-export-v2"
        ).encode()
    ).hexdigest()
    safe_name = command_name.replace("/", "_").replace("\\", "_").replace(" ", "_")
    return _runtime_icons_dir() / f"{safe_name}_{digest[:12]}.png"


def _runtime_action_icon_path(action_ref: str, icon_cache_key: str = "") -> Path:
    theme_signature = "|".join(_native_theme_signature())
    digest = hashlib.sha1(
        (
            f"{action_ref}|{icon_cache_key}|{theme_signature}"
            f"|metadata-v{_NATIVE_METADATA_CACHE_VERSION}|native-action-icon-export-v1"
        ).encode()
    ).hexdigest()
    safe_name = re.sub(r"[^A-Za-z0-9_.-]+", "_", str(action_ref or "action")).strip("_")
    if safe_name == "":
        safe_name = "action"
    return _runtime_icons_dir() / f"{safe_name[:96]}_{digest[:12]}.png"


def _persistent_action_icon_path(action_ref: str, icon_cache_key: str = "") -> Path:
    theme_signature = "|".join(_native_theme_signature())
    digest = hashlib.sha1(
        (
            f"{action_ref}|{icon_cache_key}|{theme_signature}"
            f"|metadata-v{_NATIVE_METADATA_CACHE_VERSION}|native-action-icon-export-persistent-v1"
        ).encode()
    ).hexdigest()
    safe_name = re.sub(r"[^A-Za-z0-9_.-]+", "_", str(action_ref or "action")).strip("_")
    if safe_name == "":
        safe_name = "action"
    return _persistent_icon_export_dir() / f"{safe_name[:96]}_{digest[:12]}.png"


def _persistent_icon_path(
    command_name: str, icon_hint: str = "", theme_signature: str = ""
) -> Path:
    digest = hashlib.sha1(
        (
            f"{command_name}|{icon_hint}|{theme_signature}"
            f"|metadata-v{_NATIVE_METADATA_CACHE_VERSION}|native-icon-export-persistent-v1"
        ).encode()
    ).hexdigest()
    safe_name = command_name.replace("/", "_").replace("\\", "_").replace(" ", "_")
    return _persistent_icon_export_dir() / f"{safe_name}_{digest[:12]}.png"


def _is_stable_icon_reference(icon_path: str) -> bool:
    normalized = str(icon_path or "").strip()
    if normalized == "":
        return False
    if normalized.startswith(":/"):
        return True
    if normalized.startswith("icons:"):
        try:
            if QFileInfo(normalized).exists():
                return True
        except Exception:
            pass
        resolved = NativeCommandMetadata.resolve_native_icon_path("", normalized)
        if resolved not in ["", None]:
            try:
                return Path(str(resolved)).exists()
            except Exception:
                return False
        return False
    try:
        return Path(normalized).exists()
    except Exception:
        return False


def _cached_command_metadata_is_complete(command_payload: dict) -> bool:
    payload = _coerce_dict(command_payload)
    if str(payload.get("text") or "").strip() == "":
        return False

    # Some FreeCAD commands legitimately expose no icon. Cache that negative
    # lookup, including absent iconPath, instead of rebuilding at every startup.
    if "iconPath" not in payload:
        return True

    icon_path = str(payload.get("iconPath") or "").strip()
    return icon_path == "" or _is_stable_icon_reference(icon_path)


def _export_icon(command_name: str, icon_hint: str = "", persistent: bool = False) -> str:
    theme_signature = "|".join(_native_theme_signature())
    output_path = (
        _persistent_icon_path(command_name, icon_hint, theme_signature)
        if persistent
        else _runtime_icon_path(command_name, icon_hint, theme_signature)
    )
    if output_path.exists():
        return str(output_path)

    icon = NativeCommandMetadata.load_command_icon(command_name, icon_hint)
    if icon is None or icon.isNull():
        direct_icon_path = NativeCommandMetadata.resolve_native_icon_path(
            command_name, icon_hint
        )
        if direct_icon_path not in ["", None] and Path(direct_icon_path).exists():
            return str(Path(direct_icon_path))
        return ""

    pixmap = icon.pixmap(QSize(64, 64))
    if pixmap is None or pixmap.isNull():
        return ""

    try:
        pixmap.save(str(output_path), "PNG")
    except Exception:
        output_path = None

    if output_path is not None:
        return str(output_path)

    qt_action_metadata = _qt_action_command_metadata(command_name)
    qt_icon_path = str(qt_action_metadata.get("iconPath") or "").strip()
    if qt_icon_path != "":
        return qt_icon_path

    return ""


def _normalize_action_candidate(value) -> str:
    if value in [None, ""]:
        return ""
    try:
        normalized = str(value).strip()
    except Exception:
        return ""
    if "\t" in normalized:
        normalized = normalized.split("\t", 1)[0].strip()
    return normalized


def _looks_like_command_id(value: str) -> bool:
    normalized = _normalize_action_candidate(value)
    if normalized == "":
        return False
    if "," in normalized:
        left, _, right = normalized.partition(",")
        if left.strip() != "" and right.strip().isdigit() and not any(
            character.isspace() for character in left
        ):
            return True
    return not any(character.isspace() for character in normalized)


def _action_property_text(action, name: str) -> str:
    try:
        return _normalize_action_candidate(action.property(name))
    except Exception:
        return ""


def _action_command_id(action) -> str:
    if action is None:
        return ""

    candidates = []
    for getter in [
        lambda: action.objectName(),
        lambda: _action_property_text(action, "Command"),
        lambda: _action_property_text(action, "command"),
        lambda: _action_property_text(action, "actionName"),
        lambda: action.data(),
    ]:
        try:
            candidates.append(_normalize_action_candidate(getter()))
        except Exception:
            pass

    for candidate in candidates:
        if _looks_like_command_id(candidate):
            return candidate

    associated_objects = []
    for getter in [
        lambda: action.associatedObjects(),
        lambda: action.associatedWidgets(),
    ]:
        try:
            associated_objects.extend(list(getter()))
        except Exception:
            pass
    for widget in associated_objects:
        for getter in [
            lambda widget=widget: widget.objectName(),
            lambda widget=widget: widget.property("Command"),
            lambda widget=widget: widget.property("command"),
        ]:
            try:
                candidate = _normalize_action_candidate(getter())
            except Exception:
                candidate = ""
            if _looks_like_command_id(candidate):
                return candidate
    return ""


def _clean_action_text(value) -> str:
    text = _normalize_action_candidate(value)
    if text == "":
        return ""
    return text.replace("&", "").strip()


def _action_display_text(action, fallback_command_name: str = "") -> str:
    if action is None:
        return ""

    for getter in [
        lambda: action.iconText(),
        lambda: action.text(),
        lambda: action.toolTip(),
        lambda: action.statusTip(),
    ]:
        try:
            text = _clean_action_text(getter())
        except Exception:
            text = ""
        if text == "":
            continue
        if _looks_like_technical_command_text(text, fallback_command_name) is False:
            return text

    try:
        return _clean_action_text(action.text())
    except Exception:
        return ""


def _action_shortcut_text(action) -> str:
    if action is None:
        return ""
    try:
        shortcut = action.shortcut()
        text = shortcut.toString() if hasattr(shortcut, "toString") else str(shortcut)
        return str(text or "").strip()
    except Exception:
        return ""


def _action_menu_actions(action) -> list:
    try:
        menu = action.menu()
    except Exception:
        menu = None
    if menu is None:
        return []
    try:
        return list(menu.actions())
    except Exception:
        return []


def _is_separator_action(action) -> bool:
    try:
        return bool(action.isSeparator())
    except Exception:
        return False


def _encode_command_action_trigger(
    parent_command_name: str,
    path: list[int],
    source: str = "commandActions",
) -> str:
    payload = json.dumps(
        {
            "command": str(parent_command_name or ""),
            "path": [int(index) for index in path],
            "source": str(source or "commandActions"),
        },
        ensure_ascii=True,
        separators=(",", ":"),
    ).encode("utf-8")
    encoded = base64.urlsafe_b64encode(payload).decode("ascii").rstrip("=")
    return f"__commandtab_action__:{encoded}"


def _decode_command_action_trigger(encoded_payload: str) -> tuple[str, list[int], str]:
    payload = str(encoded_payload or "").strip()
    if payload == "":
        return "", [], ""
    padding = "=" * (-len(payload) % 4)
    decoded = json.loads(base64.urlsafe_b64decode(f"{payload}{padding}").decode("utf-8"))
    if not isinstance(decoded, dict):
        return "", [], ""
    command_name = str(decoded.get("command") or "").strip()
    source = str(decoded.get("source") or "commandActions").strip()
    path = []
    for value in decoded.get("path", []):
        try:
            path.append(int(value))
        except Exception:
            return "", [], ""
    return command_name, path, source


def _toolbar_menu_actions_for_command(parent_command_name: str) -> list:
    parent_command_name = str(parent_command_name or "").strip()
    if parent_command_name == "":
        return []
    try:
        main_window = Gui.getMainWindow()
    except Exception:
        main_window = None
    if main_window is None:
        return []

    try:
        toolbars = list(main_window.findChildren(QToolBar))
    except Exception:
        toolbars = []

    for toolbar in toolbars:
        try:
            toolbar_actions = list(toolbar.actions())
        except Exception:
            toolbar_actions = []
        for toolbar_action in toolbar_actions:
            if _action_command_id(toolbar_action) != parent_command_name:
                continue

            menu_actions = _action_menu_actions(toolbar_action)
            if len(menu_actions) > 0:
                return menu_actions

            try:
                action_widget = toolbar.widgetForAction(toolbar_action)
            except Exception:
                action_widget = None
            if action_widget is None:
                continue
            menu_actions = _action_menu_actions(action_widget)
            if len(menu_actions) > 0:
                return menu_actions

        try:
            tool_buttons = list(toolbar.findChildren(QToolButton))
        except Exception:
            tool_buttons = []
        for tool_button in tool_buttons:
            action_candidates = []
            try:
                default_action = tool_button.defaultAction()
            except Exception:
                default_action = None
            if default_action is not None:
                action_candidates.append(default_action)
            try:
                action_candidates.extend(list(tool_button.actions()))
            except Exception:
                pass

            matched_button = _action_command_id(tool_button) == parent_command_name
            if not matched_button:
                for action_candidate in action_candidates:
                    if _action_command_id(action_candidate) == parent_command_name:
                        matched_button = True
                        break
            if not matched_button:
                continue

            menu_actions = _action_menu_actions(tool_button)
            if len(menu_actions) > 0:
                return menu_actions

            for action_candidate in action_candidates:
                menu_actions = _action_menu_actions(action_candidate)
                if len(menu_actions) > 0:
                    return menu_actions
    return []


def _command_action_at_path(
    parent_command_name: str,
    path: list[int],
    source: str = "commandActions",
):
    if parent_command_name == "" or len(path) == 0:
        return None
    if source == "toolbarMenu":
        current_actions = _toolbar_menu_actions_for_command(parent_command_name)
        action = None
        for index in path:
            if index < 0 or index >= len(current_actions):
                return None
            action = current_actions[index]
            current_actions = _action_menu_actions(action)
        return action

    try:
        command = Gui.Command.get(parent_command_name)
        actions = list(command.getAction()) if command is not None else []
    except Exception:
        return None

    action = None
    current_actions = actions
    for index in path:
        if index < 0 or index >= len(current_actions):
            return None
        action = current_actions[index]
        current_actions = _action_menu_actions(action)
    return action


def _trigger_encoded_command_action(encoded_payload: str) -> bool:
    parent_command_name, path, source = _decode_command_action_trigger(encoded_payload)
    action = _command_action_at_path(parent_command_name, path, source)
    if action is None:
        return False
    try:
        if bool(action.isEnabled()) is False:
            return False
    except Exception:
        pass
    try:
        action.trigger()
        return True
    except Exception:
        return False


def _export_action_icon(action_ref: str, action, persistent: bool = True) -> str:
    if action is None:
        return ""
    try:
        icon = action.icon()
    except Exception:
        icon = QIcon()
    if icon is None or icon.isNull():
        return ""

    try:
        icon_cache_key = str(int(icon.cacheKey()))
    except Exception:
        icon_cache_key = action_ref
    output_path = (
        _persistent_action_icon_path(action_ref, icon_cache_key)
        if persistent
        else _runtime_action_icon_path(action_ref, icon_cache_key)
    )
    if output_path.exists():
        return str(output_path)

    pixmap = icon.pixmap(QSize(64, 64))
    if pixmap is None or pixmap.isNull():
        pixmap = icon.pixmap(QSize(48, 48))
    if pixmap is None or pixmap.isNull():
        return ""

    try:
        if pixmap.save(str(output_path), "PNG"):
            return str(output_path)
    except Exception:
        pass
    return ""


def _command_subaction_trigger_id(
    parent_command_name: str,
    action,
    path: list[int],
    source: str = "commandActions",
) -> str:
    action_command_id = _action_command_id(action)
    if (
        action_command_id != ""
        and action_command_id != parent_command_name
        and _has_gui_command(action_command_id)
    ):
        return action_command_id
    return _encode_command_action_trigger(parent_command_name, path, source)


def _load_static_command_variant_menu_specs() -> dict[str, list[object]]:
    global _STATIC_COMMAND_VARIANT_MENU_CACHE

    if _STATIC_COMMAND_VARIANT_MENU_CACHE is not None:
        return copy.deepcopy(_STATIC_COMMAND_VARIANT_MENU_CACHE)

    specs: dict[str, list[object]] = {
        str(parent): [str(child) for child in children]
        for parent, children in _STATIC_COMMAND_VARIANT_MENUS.items()
    }
    source_path = Path(paths.addon_path("freecad_commandtab", "native", "static_variant_menus.json"))
    try:
        payload = json.loads(source_path.read_text(encoding="utf-8"))
    except Exception:
        payload = None
    if isinstance(payload, dict):
        commands = _coerce_dict(payload.get("commands"))
        for parent_command_name, children in commands.items():
            parent_command_name = str(parent_command_name or "").strip()
            if parent_command_name == "" or isinstance(children, list) is False:
                continue
            clean_children = []
            for child in children:
                if isinstance(child, dict):
                    child_id = str(child.get("id") or "").strip()
                    if child_id == "":
                        continue
                    clean_child = {"id": child_id}
                    child_text = str(child.get("text") or "").strip()
                    if child_text != "":
                        clean_child["text"] = child_text
                    clean_children.append(clean_child)
                    continue
                child_id = str(child or "").strip()
                if child_id != "":
                    clean_children.append(child_id)
            if clean_children:
                specs[parent_command_name] = clean_children

    _STATIC_COMMAND_VARIANT_MENU_CACHE = copy.deepcopy(specs)
    return specs


def _static_variant_child_command_ids(command_name: str) -> list[str]:
    command_name = str(command_name or "").strip()
    if command_name == "":
        return []
    if _STATIC_COMMAND_VARIANT_MENU_CACHE is None:
        _load_static_command_variant_menu_specs()
    specs = _STATIC_COMMAND_VARIANT_MENU_CACHE or {}
    child_ids: list[str] = []
    for child_spec in specs.get(command_name, []):
        if isinstance(child_spec, dict):
            child_id = str(child_spec.get("id") or "").strip()
        else:
            child_id = str(child_spec or "").strip()
        if child_id != "" and not child_id.startswith("__commandtab_action__:"):
            child_ids.append(child_id)
    return child_ids


def _build_static_variant_menu_entry(
    child_spec,
    command_data: dict,
) -> dict | None:
    child_text = ""
    if isinstance(child_spec, dict):
        child_command_name = str(child_spec.get("id") or "").strip()
        child_text = str(child_spec.get("text") or "").strip()
    else:
        child_command_name = str(child_spec or "").strip()
    if child_command_name == "":
        return None

    command_info = _command_info(child_command_name)
    text = child_text or _resolved_command_display_text(child_command_name, {}, command_info)
    if text == "":
        text = _humanized_command_id(child_command_name)

    icon_hint = str(command_info.get("pixmap") or "")
    icon_path = ""
    if not child_command_name.startswith("__commandtab_action__:"):
        icon_path = _export_icon(child_command_name, icon_hint, persistent=True)
    entry = {
        "type": "command",
        "id": child_command_name,
        "text": text,
        "size": "small",
        "textVisible": True,
        "iconPath": icon_path,
        "sourceWorkbenchId": str(command_data.get("sourceWorkbenchId") or ""),
        "sourceToolbarTitle": str(command_data.get("sourceToolbarTitle") or ""),
    }
    shortcut = "" if child_command_name.startswith("__commandtab_action__:") else _command_shortcut(child_command_name)
    if shortcut != "":
        entry["shortcut"] = shortcut
    return entry


def _static_variant_menu_entries(command_name: str, command_data: dict) -> list[dict]:
    command_name = str(command_name or "").strip()
    child_specs = _load_static_command_variant_menu_specs().get(command_name, [])
    if not child_specs:
        return []

    entries = []
    seen_ids = {command_name}
    for child_spec in child_specs:
        child_id = (
            str(child_spec.get("id") or "").strip()
            if isinstance(child_spec, dict)
            else str(child_spec or "").strip()
        )
        if child_id == "" or child_id in seen_ids:
            continue
        entry = _build_static_variant_menu_entry(child_spec, command_data)
        if entry is None:
            continue
        seen_ids.add(child_id)
        entries.append(entry)
    return entries


def _cached_or_static_variant_menu_entries(command_name: str, command_data: dict) -> list[dict]:
    entries = _cached_variant_menu_entries(command_name)
    static_entries = _static_variant_menu_entries(command_name, command_data)
    if not entries:
        return static_entries
    if not static_entries:
        return entries

    seen_ids = {
        str(entry.get("id") or "").strip()
        for entry in entries
        if isinstance(entry, dict)
    }
    merged_entries = list(entries)
    for static_entry in static_entries:
        static_id = str(static_entry.get("id") or "").strip()
        if static_id == "" or static_id in seen_ids:
            continue
        merged_entries.append(static_entry)
        seen_ids.add(static_id)
    return merged_entries


def _build_command_subaction_entry(
    parent_command_name: str,
    action,
    path: list[int],
    command_data: dict,
    source: str = "commandActions",
) -> dict | None:
    if action is None or _is_separator_action(action):
        return None

    menu_actions = _action_menu_actions(action)
    if len(menu_actions) > 0:
        return None

    trigger_id = _command_subaction_trigger_id(parent_command_name, action, path, source)
    text = _action_display_text(action, parent_command_name)
    if text == "":
        text = _humanized_command_id(trigger_id)

    icon_path = _export_action_icon(trigger_id, action)
    if icon_path == "" and not trigger_id.startswith("__commandtab_action__:"):
        icon_path = _export_icon(trigger_id, "")

    entry = {
        "type": "command",
        "id": trigger_id,
        "text": text,
        "size": "small",
        "textVisible": True,
        "iconPath": icon_path,
        "sourceWorkbenchId": str(command_data.get("sourceWorkbenchId") or ""),
        "sourceToolbarTitle": str(command_data.get("sourceToolbarTitle") or ""),
    }

    shortcut = _action_shortcut_text(action)
    if shortcut == "" and not trigger_id.startswith("__commandtab_action__:"):
        shortcut = _command_shortcut(trigger_id)
    if shortcut != "":
        entry["shortcut"] = shortcut
    return entry


def _append_command_subaction_entries(
    entries: list[dict],
    parent_command_name: str,
    action,
    path: list[int],
    command_data: dict,
    seen_ids: set[str],
    source: str = "commandActions",
) -> None:
    if action is None or _is_separator_action(action):
        return

    menu_actions = _action_menu_actions(action)
    if len(menu_actions) > 0:
        for index, child_action in enumerate(menu_actions):
            _append_command_subaction_entries(
                entries,
                parent_command_name,
                child_action,
                [*path, index],
                command_data,
                seen_ids,
                source,
            )
        return

    entry = _build_command_subaction_entry(
        parent_command_name,
        action,
        path,
        command_data,
        source,
    )
    if entry is None:
        return
    entry_id = str(entry.get("id") or "")
    if entry_id == "" or entry_id in seen_ids:
        return
    seen_ids.add(entry_id)
    entries.append(entry)


def _command_subaction_entries(command_name: str, command_data: dict) -> list[dict]:
    if command_name.startswith("__commandtab_action__:"):
        return []
    try:
        command = Gui.Command.get(command_name)
        actions = list(command.getAction()) if command is not None else []
    except Exception:
        return _cached_or_static_variant_menu_entries(command_name, command_data)
    if len(actions) == 0:
        return _cached_or_static_variant_menu_entries(command_name, command_data)

    entries: list[dict] = []
    seen_ids: set[str] = {command_name}
    include_plain_command_actions = len(actions) > 1
    for index, action in enumerate(actions):
        menu_actions = _action_menu_actions(action)
        if len(menu_actions) == 0 and not include_plain_command_actions:
            continue
        _append_command_subaction_entries(
            entries,
            command_name,
            action,
            [index],
            command_data,
            seen_ids,
        )

    for index, action in enumerate(_toolbar_menu_actions_for_command(command_name)):
        _append_command_subaction_entries(
            entries,
            command_name,
            action,
            [index],
            command_data,
            seen_ids,
            "toolbarMenu",
        )
    if entries:
        _write_variant_menu_cache_entry(command_name, entries)
        return entries
    return _cached_or_static_variant_menu_entries(command_name, command_data)


def _command_subaction_signature(menu_commands: list[dict]) -> tuple[tuple[str, str], ...]:
    signature = []
    for command in menu_commands:
        signature.append((str(command.get("id") or ""), str(command.get("text") or "")))
    return tuple(signature)


def _variant_menu_cache_key() -> dict[str, object]:
    return {
        "cacheVersion": _NATIVE_VARIANT_MENU_CACHE_VERSION,
        "metadataCacheVersion": _NATIVE_METADATA_CACHE_VERSION,
        "environmentSignature": _native_metadata_environment_signature(),
        "themeSignature": list(_native_theme_signature()),
    }


def _read_variant_menu_cache() -> dict[str, list[dict]]:
    expected_key = _variant_menu_cache_key()
    if _coerce_dict(_VARIANT_MENU_CACHE_MEMORY.get("key")) == expected_key:
        return copy.deepcopy(_coerce_dict(_VARIANT_MENU_CACHE_MEMORY.get("commands")))

    cache_path = _persistent_variant_menu_cache_path()
    if cache_path.exists() is False:
        return {}
    try:
        payload = json.loads(cache_path.read_text(encoding="utf-8"))
    except Exception:
        return {}
    if isinstance(payload, dict) is False:
        return {}
    if _coerce_dict(payload.get("key")) != expected_key:
        return {}
    commands = _coerce_dict(payload.get("commands"))
    result: dict[str, list[dict]] = {}
    for command_name, entries in commands.items():
        normalized_name = str(command_name or "").strip()
        if normalized_name == "" or isinstance(entries, list) is False:
            continue
        clean_entries = []
        for entry in entries:
            entry_object = _coerce_dict(entry)
            entry_id = str(entry_object.get("id") or "").strip()
            if entry_id == "":
                continue
            entry_object["type"] = str(entry_object.get("type") or "command")
            entry_object["id"] = entry_id
            clean_entries.append(entry_object)
        if clean_entries:
            result[normalized_name] = clean_entries
    _VARIANT_MENU_CACHE_MEMORY["key"] = expected_key
    _VARIANT_MENU_CACHE_MEMORY["commands"] = copy.deepcopy(result)
    return result


def _cached_variant_menu_entries(command_name: str) -> list[dict]:
    normalized_name = str(command_name or "").strip()
    if normalized_name == "":
        return []
    entries = _read_variant_menu_cache().get(normalized_name, [])
    if not entries:
        return []
    return copy.deepcopy(entries)


def _write_variant_menu_cache_entry(command_name: str, entries: list[dict]) -> None:
    normalized_name = str(command_name or "").strip()
    if normalized_name == "" or not entries:
        return
    clean_entries = []
    for entry in entries:
        entry_object = _coerce_dict(entry)
        entry_id = str(entry_object.get("id") or "").strip()
        if entry_id == "":
            continue
        clean_entry = {
            "type": str(entry_object.get("type") or "command"),
            "id": entry_id,
            "text": str(entry_object.get("text") or entry_id),
            "size": str(entry_object.get("size") or "small"),
            "textVisible": bool(entry_object.get("textVisible", True)),
            "iconPath": str(entry_object.get("iconPath") or ""),
            "sourceWorkbenchId": str(entry_object.get("sourceWorkbenchId") or ""),
            "sourceToolbarTitle": str(entry_object.get("sourceToolbarTitle") or ""),
        }
        shortcut = str(entry_object.get("shortcut") or "").strip()
        if shortcut != "":
            clean_entry["shortcut"] = shortcut
        nested_entries = _coerce_list(entry_object.get("menuCommands", []))
        if nested_entries:
            clean_entry["menuCommands"] = copy.deepcopy(nested_entries)
        clean_entries.append(clean_entry)
    if not clean_entries:
        return

    cache_path = _persistent_variant_menu_cache_path()
    key = _variant_menu_cache_key()
    current_commands = _read_variant_menu_cache()
    current_commands[normalized_name] = clean_entries
    _VARIANT_MENU_CACHE_MEMORY["key"] = key
    _VARIANT_MENU_CACHE_MEMORY["commands"] = copy.deepcopy(current_commands)
    tmp_path = cache_path.with_suffix(".tmp")
    try:
        tmp_path.write_text(
            json.dumps(
                {
                    "key": key,
                    "commands": current_commands,
                },
                ensure_ascii=True,
                separators=(",", ":"),
            ),
            encoding="utf-8",
        )
        tmp_path.replace(cache_path)
    except Exception:
        try:
            tmp_path.unlink(missing_ok=True)
        except Exception:
            pass


def _resolve_structure_path() -> Path:
    # The native commandtab must always read the same editable structure file that
    # customization dialogs update. Otherwise visibility/layout edits can be
    # written successfully but never reflected in the live model.
    return _editable_structure_path()


def _editable_structure_path() -> Path:
    configured_path = Path(str(getattr(Parameters_CommandTab, "COMMANDTAB_STRUCTURE_JSON", "") or "").strip())
    if str(configured_path) not in ["", "."]:
        if configured_path.exists():
            return configured_path
        configured_path.parent.mkdir(parents=True, exist_ok=True)
        source = Path(paths.addon_path("CreateStructure.txt"))
        if source.exists():
            shutil.copy(source, configured_path)
            return configured_path

    preferred = Path(paths.user_state_path("CommandTabStructure.json"))
    if preferred.exists():
        return preferred

    source = Path(paths.addon_path("CreateStructure.txt"))
    if source.exists():
        preferred.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy(source, preferred)
        return preferred
    raise FileNotFoundError("Editable commandtab structure path not available")


def _command_list_from_toolbar_items(items) -> list[str]:
    commands: list[str] = []
    separator_index = 0
    try:
        iterable = list(items)
    except Exception:
        return commands

    for item in iterable:
        command_name = str(item or "").strip()
        if command_name == "":
            continue
        if _is_separator_command_id(command_name):
            command_name = f"{separator_index}_separator_dynamic"
            separator_index += 1
        commands.append(command_name)
    return commands


def _is_common_dynamic_workbench_toolbar(toolbar_id: str) -> bool:
    normalized = str(toolbar_id or "").replace("&", "").strip().lower()
    return normalized in _DYNAMIC_WORKBENCH_COMMON_TOOLBARS


def _dynamic_workbench_structure_from_toolbar_items(toolbar_items) -> dict:
    if not isinstance(toolbar_items, dict):
        return {}

    toolbars: dict[str, object] = {"order": []}
    ordered_toolbar_ids: list[str] = []
    seen_toolbar_ids: set[str] = set()

    for toolbar_title, items in toolbar_items.items():
        toolbar_id = str(toolbar_title or "").replace("&", "").strip()
        if toolbar_id == "":
            continue
        commands = _command_list_from_toolbar_items(items)
        if len(commands) == 0:
            continue
        if _is_common_dynamic_workbench_toolbar(toolbar_id):
            continue

        unique_toolbar_id = toolbar_id
        suffix = 2
        while unique_toolbar_id in seen_toolbar_ids:
            unique_toolbar_id = f"{toolbar_id} {suffix}"
            suffix += 1
        seen_toolbar_ids.add(unique_toolbar_id)
        ordered_toolbar_ids.append(unique_toolbar_id)
        toolbars[unique_toolbar_id] = {
            "title": toolbar_id,
            "order": commands,
            "commands": {
                command_name: {"size": "small"}
                for command_name in commands
                if _is_separator_command_id(command_name) is False
            },
            "Enabled": True,
        }

    if len(ordered_toolbar_ids) == 0:
        return {}

    toolbars["order"] = ordered_toolbar_ids
    return {"toolbars": toolbars}


def _workbench_toolbar_items(workbench_name: str) -> dict:
    try:
        workbench = Gui.getWorkbench(workbench_name)
    except Exception:
        workbench = None
    if workbench is None:
        workbench = _available_workbenches().get(workbench_name)
    if workbench is None:
        return {}

    try:
        toolbar_items = workbench.getToolbarItems()
    except Exception:
        return {}
    if not isinstance(toolbar_items, dict):
        return {}
    return toolbar_items


def _structure_workbench_has_panels(structure: dict, workbench_name: str) -> bool:
    workbench_data = _coerce_dict(
        _coerce_dict(structure.get("workbenches", {})).get(workbench_name, {})
    )
    toolbars = _coerce_dict(workbench_data.get("toolbars", {}))
    for panel_id, panel_data in toolbars.items():
        if str(panel_id) == "order":
            continue
        if not isinstance(panel_data, dict):
            continue
        if bool(panel_data.get("Enabled", True)) is False:
            continue
        if len(_coerce_list(panel_data.get("order", []))) > 0:
            return True
    return False


def _dynamic_workbench_signature(workbench_data: dict) -> str:
    toolbars = _coerce_dict(workbench_data.get("toolbars", {}))
    signature_payload: list[object] = []
    for toolbar_id in _coerce_list(toolbars.get("order", [])):
        toolbar_key = str(toolbar_id or "").strip()
        if toolbar_key == "":
            continue
        toolbar_data = _coerce_dict(toolbars.get(toolbar_key, {}))
        signature_payload.append(
            [
                toolbar_key,
                [
                    str(command_name or "").strip()
                    for command_name in _coerce_list(toolbar_data.get("order", []))
                    if str(command_name or "").strip() != ""
                ],
            ]
        )
    return hashlib.sha1(
        json.dumps(signature_payload, ensure_ascii=True, separators=(",", ":")).encode("utf-8")
    ).hexdigest()


def _merge_dynamic_workbench_structure(existing_workbench: dict, captured_workbench: dict) -> bool:
    existing_toolbars = _coerce_dict(existing_workbench.get("toolbars", {}))
    captured_toolbars = _coerce_dict(captured_workbench.get("toolbars", {}))
    existing_order = [
        str(toolbar_id or "").strip()
        for toolbar_id in _coerce_list(existing_toolbars.get("order", []))
        if str(toolbar_id or "").strip() != ""
    ]
    dirty = False

    for toolbar_id in list(existing_order):
        if toolbar_id in captured_toolbars:
            continue
        if _is_common_dynamic_workbench_toolbar(toolbar_id) is False:
            continue
        existing_order.remove(toolbar_id)
        existing_toolbars.pop(toolbar_id, None)
        dirty = True

    for captured_toolbar_id in _coerce_list(captured_toolbars.get("order", [])):
        toolbar_id = str(captured_toolbar_id or "").strip()
        if toolbar_id == "":
            continue
        captured_toolbar = _coerce_dict(captured_toolbars.get(toolbar_id, {}))
        if toolbar_id not in existing_toolbars:
            existing_toolbars[toolbar_id] = copy.deepcopy(captured_toolbar)
            if toolbar_id not in existing_order:
                existing_order.append(toolbar_id)
            dirty = True
            continue

        existing_toolbar = _coerce_dict(existing_toolbars.get(toolbar_id, {}))
        existing_commands = _coerce_dict(existing_toolbar.get("commands", {}))
        existing_command_order = [
            str(command_name or "").strip()
            for command_name in _coerce_list(existing_toolbar.get("order", []))
            if str(command_name or "").strip() != ""
        ]
        for command_name in _coerce_list(captured_toolbar.get("order", [])):
            command_id = str(command_name or "").strip()
            if command_id == "" or command_id in existing_command_order:
                continue
            existing_command_order.append(command_id)
            if _is_separator_command_id(command_id) is False:
                captured_commands = _coerce_dict(captured_toolbar.get("commands", {}))
                existing_commands[command_id] = copy.deepcopy(
                    _coerce_dict(captured_commands.get(command_id, {"size": "small"}))
                )
            dirty = True

        if existing_command_order != _coerce_list(existing_toolbar.get("order", [])):
            existing_toolbar["order"] = existing_command_order
        if existing_commands:
            existing_toolbar["commands"] = existing_commands
        if str(existing_toolbar.get("title") or "").strip() == "":
            existing_toolbar["title"] = str(captured_toolbar.get("title") or toolbar_id)
        if "Enabled" not in existing_toolbar:
            existing_toolbar["Enabled"] = True
        existing_toolbars[toolbar_id] = existing_toolbar

    if existing_order != _coerce_list(existing_toolbars.get("order", [])):
        existing_toolbars["order"] = existing_order
        dirty = True
    existing_workbench["toolbars"] = existing_toolbars
    return dirty


def _clear_structure_and_model_caches() -> None:
    _STRUCTURE_CACHE.clear()
    _BASE_MODEL_CACHE.clear()
    _WORKBENCH_PANEL_CACHE.clear()
    _WORKBENCH_PAYLOAD_CACHE.clear()
    _BOOTSTRAP_PAYLOAD_CACHE.clear()
    _WORKBENCH_BOOTSTRAP_CACHE.clear()
    _WORKBENCH_BOOTSTRAP_STATE_CACHE.clear()
    _MODEL_PAYLOAD_CACHE.clear()


def _ensure_dynamic_workbench_structure(workbench_name: str) -> bool:
    workbench_name = str(workbench_name or "").strip()
    if workbench_name == "" or workbench_name in {"NoneWorkbench"}:
        return False
    if _is_available_workbench(workbench_name) is False:
        return False

    with StartupTrace.span(
        "bridge.ensure_dynamic_workbench_structure",
        workbench=workbench_name,
    ):
        try:
            structure_path = _editable_structure_path()
            structure = json.loads(structure_path.read_text(encoding="utf-8"))
        except Exception:
            return False
        if not isinstance(structure, dict):
            return False
        if _is_ignored_workbench(structure, workbench_name):
            return False

        workbenches = _coerce_dict(structure.get("workbenches", {}))
        existing_workbench = _coerce_dict(workbenches.get(workbench_name, {}))
        dynamic_workbenches = _coerce_dict(structure.get("dynamicWorkbenches", {}))
        dynamic_metadata = _coerce_dict(dynamic_workbenches.get(workbench_name, {}))
        is_dynamic_workbench = (
            str(dynamic_metadata.get("source") or "").strip()
            == "Gui.Workbench.getToolbarItems"
        )
        existing_has_panels = _structure_workbench_has_panels(structure, workbench_name)
        if existing_has_panels and is_dynamic_workbench is False:
            return False

        captured_workbench = _dynamic_workbench_structure_from_toolbar_items(
            _workbench_toolbar_items(workbench_name)
        )
        if not captured_workbench:
            return False

        captured_signature = _dynamic_workbench_signature(captured_workbench)
        if existing_has_panels:
            dirty = _merge_dynamic_workbench_structure(existing_workbench, captured_workbench)
            if (
                dirty is False
                and str(dynamic_metadata.get("signature") or "") == captured_signature
            ):
                return False
            workbenches[workbench_name] = existing_workbench
        else:
            workbenches[workbench_name] = captured_workbench
            dirty = True
        structure["workbenches"] = workbenches
        dynamic_workbenches = _coerce_dict(structure.get("dynamicWorkbenches", {}))
        dynamic_workbenches[workbench_name] = {
            "source": "Gui.Workbench.getToolbarItems",
            "toolbarCount": len(_coerce_dict(captured_workbench.get("toolbars", {})).get("order", [])),
            "signature": captured_signature,
        }
        structure["dynamicWorkbenches"] = dynamic_workbenches

        try:
            structure_path.write_text(
                json.dumps(structure, indent=4, ensure_ascii=False),
                encoding="utf-8",
            )
        except Exception:
            return False

        _clear_structure_and_model_caches()
        StartupTrace.mark(
            "bridge.dynamic_workbench_structure_saved",
            workbench=workbench_name,
            toolbarCount=int(dynamic_workbenches[workbench_name].get("toolbarCount", 0)),
        )
        return True


def _ensure_initialized_available_workbench_structures() -> int:
    saved_count = 0
    try:
        available_names = list(_available_workbenches().keys())
    except Exception:
        return saved_count
    for workbench_name in available_names:
        try:
            if _ensure_dynamic_workbench_structure(str(workbench_name)):
                saved_count += 1
        except Exception:
            continue
    return saved_count


def _decode_native_design_payload(encoded_payload: str) -> dict:
    payload = str(encoded_payload or "").strip()
    if payload == "":
        raise ValueError("Native design payload is empty")
    padding = "=" * (-len(payload) % 4)
    raw_bytes = base64.urlsafe_b64decode(f"{payload}{padding}")
    decoded = json.loads(raw_bytes.decode("utf-8"))
    if not isinstance(decoded, dict):
        raise ValueError("Native design payload must be a JSON object")
    return decoded


def _decode_base64_json_payload(encoded_payload: str) -> dict:
    payload = str(encoded_payload or "").strip()
    if payload == "":
        raise ValueError("Native preferences payload is empty")
    padding = "=" * (-len(payload) % 4)
    raw_bytes = base64.urlsafe_b64decode(f"{payload}{padding}")
    decoded = json.loads(raw_bytes.decode("utf-8"))
    if not isinstance(decoded, dict):
        raise ValueError("Native preferences payload must be a JSON object")
    return decoded


def _decode_native_design_argument(encoded_value: str) -> str:
    payload = str(encoded_value or "").strip()
    if payload == "":
        raise ValueError("Native design argument is empty")
    padding = "=" * (-len(payload) % 4)
    raw_bytes = base64.urlsafe_b64decode(f"{payload}{padding}")
    return raw_bytes.decode("utf-8")


def _apply_native_design_payload(encoded_payload: str) -> bool:
    payload = _decode_native_design_payload(encoded_payload)
    structure_path = _editable_structure_path()
    payload_json = json.dumps(payload, ensure_ascii=True, separators=(",", ":"))

    native_module = _load_native_extension_module()
    if native_module is not None:
        apply_customization_to_structure = getattr(
            native_module, "apply_customization_to_structure", None
        )
        if callable(apply_customization_to_structure):
            try:
                apply_customization_to_structure(payload_json, str(structure_path))
                _STRUCTURE_CACHE.clear()
                _invalidate_native_model_caches()
                return refresh_native_commandtab(force=True, include_all_panels=True)
            except Exception:
                pass

    try:
        native_library = _load_native_library()
    except Exception:
        native_library = None

    if (
        native_library is not None
        and hasattr(native_library, "freecad_commandtab_native_apply_customization_to_structure")
    ):
        applied = bool(
            native_library.freecad_commandtab_native_apply_customization_to_structure(
                payload_json.encode("utf-8"),
                str(structure_path).encode("utf-8"),
            )
        )
        if applied:
            _STRUCTURE_CACHE.clear()
            _invalidate_native_model_caches()
            return refresh_native_commandtab(force=True, include_all_panels=True)

    structure = json.loads(structure_path.read_text(encoding="utf-8"))
    if isinstance(structure, dict) is False:
        raise ValueError("CommandTab structure root must be a JSON object")
    if not isinstance(structure, dict):
        raise ValueError("CommandTab structure must be a JSON object")

    quick_access_commands = []
    for command_name in payload.get("quickAccess", []):
        command_name = str(command_name or "").strip()
        if command_name != "":
            quick_access_commands.append(command_name)
    structure["quickAccessCommands"] = quick_access_commands

    dropdown_buttons = {}

    def _scoped_panel_key(workbench_id: str, panel_id: str) -> str:
        wb = str(workbench_id or "").strip()
        panel = str(panel_id or "").strip()
        if wb == "" or panel == "":
            return ""
        return f"{wb}::{panel}"

    existing_ignored_toolbars = {
        str(item or "").strip()
        for item in structure.get("ignoredToolbars", [])
        if str(item or "").strip() != ""
    }
    represented_panel_ids = set()
    represented_scoped_panels = set()
    hidden_panels = set()
    for panel_payload in payload.get("panelVisibility", []):
        if not isinstance(panel_payload, dict):
            continue
        panel_id = str(panel_payload.get("id") or "").strip()
        workbench_id = str(panel_payload.get("workbenchId") or "").strip()
        if panel_id == "":
            continue
        represented_panel_ids.add(panel_id)
        scoped_key = _scoped_panel_key(workbench_id, panel_id)
        if scoped_key != "":
            represented_scoped_panels.add(scoped_key)
        if bool(panel_payload.get("visible", True)) is False:
            hidden_panels.add(scoped_key or panel_id)
    preserved_ignored_toolbars = {
        item
        for item in existing_ignored_toolbars
        if (
            (item.find("::") >= 0 and item not in represented_scoped_panels)
            or (item.find("::") < 0 and item not in represented_panel_ids)
        )
    }
    structure["ignoredToolbars"] = sorted(preserved_ignored_toolbars | hidden_panels)

    hidden_workbenches = {
        str(item or "").strip()
        for item in payload.get("hiddenWorkbenches", [])
        if str(item or "").strip() != ""
    }
    if not hidden_workbenches:
        for workbench_payload in payload.get("workbenchVisibility", []):
            if not isinstance(workbench_payload, dict):
                continue
            workbench_id = str(workbench_payload.get("id") or "").strip()
            if workbench_id == "":
                continue
            if bool(workbench_payload.get("visible", True)) is False:
                hidden_workbenches.add(workbench_id)
    structure["ignoredWorkbenches"] = sorted(hidden_workbenches)

    for dropdown_payload in payload.get("dropdowns", []):
        if not isinstance(dropdown_payload, dict):
            continue
        dropdown_id = str(dropdown_payload.get("id") or "").strip()
        if dropdown_id == "":
            continue
        existing_dropdown_items = _coerce_list(
            _coerce_dict(structure.get("dropdownButtons", {})).get(dropdown_id, [])
        )
        existing_dropdown_sources = {}
        for item in existing_dropdown_items:
            if isinstance(item, (list, tuple)) and len(item) >= 2:
                existing_dropdown_sources[str(item[0] or "").strip()] = str(item[1] or "")
        dropdown_items = []
        for menu_command in dropdown_payload.get("commands", []):
            if not isinstance(menu_command, dict):
                continue
            menu_command_id = str(menu_command.get("id") or "").strip()
            if menu_command_id == "":
                continue
            source_workbench_id = str(menu_command.get("sourceWorkbenchId") or "").strip()
            if source_workbench_id == "":
                source_workbench_id = str(existing_dropdown_sources.get(menu_command_id) or "General")
            dropdown_items.append([menu_command_id, source_workbench_id])
        dropdown_buttons[dropdown_id] = dropdown_items

    payload_workbenches = {}
    for workbench_payload in payload.get("workbenches", []):
        if not isinstance(workbench_payload, dict):
            continue
        workbench_id = str(workbench_payload.get("id") or "").strip()
        if workbench_id != "":
            payload_workbenches[workbench_id] = workbench_payload

    structure_workbenches = _coerce_dict(structure.get("workbenches", {}))
    structure_custom_toolbars = _coerce_dict(structure.get("customToolbars", {}))
    structure_new_panels = _coerce_dict(structure.get("newPanels", {}))
    for workbench_id, workbench_data in structure_workbenches.items():
        workbench_id = str(workbench_id)
        if not isinstance(workbench_data, dict):
            continue
        payload_workbench = payload_workbenches.get(workbench_id)
        if not isinstance(payload_workbench, dict):
            continue

        existing_toolbars = _coerce_dict(workbench_data.get("toolbars", {}))
        existing_custom_panels = _coerce_dict(structure_custom_toolbars.get(workbench_id, {}))
        reordered_toolbars = {}
        ordered_panel_ids = []
        seen_standard_panels = set()
        rebuilt_custom_panels = {}
        rebuilt_new_panels: dict[str, dict] = {}
        for panel_payload in payload_workbench.get("panels", []):
            if not isinstance(panel_payload, dict):
                continue
            panel_id = str(panel_payload.get("id") or "").strip()
            if panel_id == "":
                continue
            ordered_panel_ids.append(panel_id)

            source_type = str(panel_payload.get("sourceType") or "toolbar").strip().lower()
            if source_type not in ["toolbar", "custom", "new"]:
                source_type = "toolbar"
            source_workbench_id = str(
                panel_payload.get("sourceWorkbenchId")
                or ("Global" if source_type == "new" and workbench_id == "Global" else workbench_id)
            ).strip()
            if source_workbench_id == "":
                source_workbench_id = workbench_id

            command_entries = []
            command_order = []
            separator_index = 0

            for command_payload in panel_payload.get("commands", []):
                if not isinstance(command_payload, dict):
                    continue
                command_type = str(command_payload.get("type") or "command").strip().lower()
                if command_type == "separator":
                    command_id = str(command_payload.get("id") or "").strip()
                    if command_id == "":
                        command_id = f"{separator_index}_separator_{workbench_id}"
                    separator_index += 1
                    command_entries.append((command_id, command_type, {}))
                    continue

                command_id = str(command_payload.get("id") or "").strip()
                if command_id == "":
                    continue
                command_entries.append((command_id, command_type, _coerce_dict(command_payload)))

            if source_type == "custom":
                existing_custom_commands = _coerce_dict(
                    _coerce_dict(existing_custom_panels.get(panel_id, {})).get("commands", {})
                )
                custom_commands = {}
                for command_id, command_type, command_payload in command_entries:
                    if command_type == "separator":
                        continue
                    source_toolbar_title = str(command_payload.get("sourceToolbarTitle") or "").strip()
                    if source_toolbar_title == "":
                        source_toolbar_title = str(existing_custom_commands.get(command_id) or "")
                    custom_commands[command_id] = source_toolbar_title
                rebuilt_custom_panels[panel_id] = {"commands": custom_commands}
                continue

            if source_type == "new":
                new_panel_commands = []
                for command_id, _command_type, command_payload in command_entries:
                    source_command_workbench_id = str(
                        command_payload.get("sourceWorkbenchId") or source_workbench_id or "General"
                    ).strip()
                    if source_command_workbench_id == "":
                        source_command_workbench_id = "General"
                    new_panel_commands.append([command_id, source_command_workbench_id])
                rebuilt_new_panels.setdefault(source_workbench_id, {})[panel_id] = new_panel_commands
                continue

            panel_data = _coerce_dict(existing_toolbars.get(panel_id, {}))
            existing_commands = _coerce_dict(panel_data.get("commands", {}))
            reordered_commands = _coerce_dict(existing_commands)
            seen_standard_panels.add(panel_id)
            command_order = []
            for command_id, command_type, command_payload in command_entries:
                if command_type == "separator":
                    command_order.append(command_id)
                    reordered_commands.setdefault(command_id, {})
                    continue

                command_order.append(command_id)
                command_data = _coerce_dict(reordered_commands.get(command_id, {}))
                size = str(command_payload.get("size") or command_data.get("size") or "small").strip().lower()
                if size not in ["small", "medium", "large"]:
                    size = "small"
                command_data["size"] = size
                if "text" in command_payload:
                    command_data["text"] = str(command_payload.get("text") or "")
                reordered_commands[command_id] = command_data

            panel_data["order"] = command_order
            panel_data["commands"] = reordered_commands
            panel_data["title"] = str(panel_payload.get("title") or panel_data.get("title") or panel_id)
            panel_data["Enabled"] = True
            reordered_toolbars[panel_id] = panel_data

        for panel_id, panel_data in existing_toolbars.items():
            panel_id = str(panel_id)
            if panel_id == "order":
                continue
            if panel_id in seen_standard_panels:
                continue
            if isinstance(panel_data, dict):
                hidden_panel = _coerce_dict(panel_data)
                hidden_panel["Enabled"] = False
                reordered_toolbars[panel_id] = hidden_panel
            else:
                reordered_toolbars[panel_id] = panel_data

        reordered_toolbars["order"] = ordered_panel_ids
        workbench_data["toolbars"] = reordered_toolbars
        structure_workbenches[workbench_id] = workbench_data
        structure_custom_toolbars[workbench_id] = rebuilt_custom_panels
        structure_new_panels[workbench_id] = rebuilt_new_panels.get(workbench_id, {})
        if "Global" in structure_new_panels or "Global" in rebuilt_new_panels:
            structure_new_panels["Global"] = rebuilt_new_panels.get("Global", {})

    structure["workbenches"] = structure_workbenches
    structure["customToolbars"] = structure_custom_toolbars
    structure["newPanels"] = structure_new_panels
    structure["dropdownButtons"] = dropdown_buttons
    structure_path.write_text(json.dumps(structure, indent=4), encoding="utf-8")
    _STRUCTURE_CACHE.clear()
    _invalidate_native_model_caches()
    return refresh_native_commandtab(force=True, include_all_panels=True)


def _refresh_native_design_state() -> bool:
    _STRUCTURE_CACHE.clear()
    _invalidate_native_model_caches()
    return refresh_native_commandtab(force=True, include_all_panels=True)


def _export_native_design_layout(encoded_target_path: str) -> bool:
    target_path = Path(_decode_native_design_argument(encoded_target_path))
    target_path.parent.mkdir(parents=True, exist_ok=True)
    structure_path = _editable_structure_path()
    native_module = _load_native_extension_module()
    if native_module is not None:
        export_structure_copy = getattr(native_module, "export_structure_copy", None)
        if callable(export_structure_copy):
            try:
                export_structure_copy(str(structure_path), str(target_path))
                return True
            except Exception:
                pass
    try:
        native_library = _load_native_library()
    except Exception:
        native_library = None
    if (
        native_library is not None
        and hasattr(native_library, "freecad_commandtab_native_export_structure_copy")
    ):
        exported = bool(
            native_library.freecad_commandtab_native_export_structure_copy(
                str(structure_path).encode("utf-8"),
                str(target_path).encode("utf-8"),
            )
        )
        if exported:
            return True
    shutil.copy(structure_path, target_path)
    return True


def _import_native_design_layout(encoded_source_path: str) -> bool:
    source_path = Path(_decode_native_design_argument(encoded_source_path))
    structure_path = _editable_structure_path()
    native_module = _load_native_extension_module()
    if native_module is not None:
        import_structure_copy = getattr(native_module, "import_structure_copy", None)
        if callable(import_structure_copy):
            try:
                import_structure_copy(str(source_path), str(structure_path))
                return _refresh_native_design_state()
            except Exception:
                pass
    try:
        native_library = _load_native_library()
    except Exception:
        native_library = None
    if (
        native_library is not None
        and hasattr(native_library, "freecad_commandtab_native_import_structure_copy")
    ):
        imported = bool(
            native_library.freecad_commandtab_native_import_structure_copy(
                str(source_path).encode("utf-8"),
                str(structure_path).encode("utf-8"),
            )
        )
        if imported:
            return _refresh_native_design_state()
    payload = json.loads(source_path.read_text(encoding="utf-8"))
    if not isinstance(payload, dict):
        raise ValueError("Imported commandtab structure must be a JSON object")
    structure_path.write_text(json.dumps(payload, indent=4), encoding="utf-8")
    return _refresh_native_design_state()


def _reset_native_design_layout() -> bool:
    target_path = _editable_structure_path()
    default_path = target_path.with_name("CommandTabStructure_default.json")
    if not default_path.exists():
        default_path = Path(paths.addon_path("CreateStructure.txt"))
    if not default_path.exists():
        raise FileNotFoundError("Default commandtab structure file not found")
    native_module = _load_native_extension_module()
    if native_module is not None:
        reset_structure_copy = getattr(native_module, "reset_structure_copy", None)
        if callable(reset_structure_copy):
            try:
                reset_structure_copy(str(default_path), str(target_path))
                return _refresh_native_design_state()
            except Exception:
                pass
    try:
        native_library = _load_native_library()
    except Exception:
        native_library = None
    if (
        native_library is not None
        and hasattr(native_library, "freecad_commandtab_native_reset_structure_copy")
    ):
        reset_ok = bool(
            native_library.freecad_commandtab_native_reset_structure_copy(
                str(default_path).encode("utf-8"),
                str(target_path).encode("utf-8"),
            )
        )
        if reset_ok:
            return _refresh_native_design_state()
    shutil.copy(default_path, target_path)
    return _refresh_native_design_state()


def _restore_native_design_layout() -> bool:
    backup_dir = Path(str(getattr(Parameters_CommandTab, "BACKUP_LOCATION", "") or "").strip())
    if not backup_dir.exists():
        return False
    target_path = _editable_structure_path()
    native_module = _load_native_extension_module()
    if native_module is not None:
        restore_structure_from_backup = getattr(
            native_module, "restore_structure_from_backup", None
        )
        if callable(restore_structure_from_backup):
            try:
                restore_structure_from_backup(str(backup_dir), str(target_path))
                return _refresh_native_design_state()
            except Exception:
                pass
    try:
        native_library = _load_native_library()
    except Exception:
        native_library = None
    if (
        native_library is not None
        and hasattr(native_library, "freecad_commandtab_native_restore_structure_from_backup")
    ):
        restored = bool(
            native_library.freecad_commandtab_native_restore_structure_from_backup(
                str(backup_dir).encode("utf-8"),
                str(target_path).encode("utf-8"),
            )
        )
        if restored:
            return _refresh_native_design_state()
    candidates = sorted(
        [path for path in backup_dir.glob("CommandTabStructure*.json") if path.is_file()],
        key=lambda path: path.stat().st_mtime_ns,
        reverse=True,
    )
    if not candidates:
        return False
    shutil.copy(candidates[0], target_path)
    return _refresh_native_design_state()


def _toggle_quick_access_command(encoded_id: str) -> None:
    padding = "=" * (-len(encoded_id) % 4)
    command_id = base64.urlsafe_b64decode(f"{encoded_id}{padding}").decode("utf-8").strip()
    if not command_id:
        return

    structure_path = _editable_structure_path()
    try:
        with open(structure_path, encoding="utf-8") as fh:
            structure = json.load(fh)
    except Exception:
        structure = {}

    qa_list = list(structure.get("quickAccessCommands", []))
    if command_id in qa_list:
        qa_list = [c for c in qa_list if c != command_id]
    else:
        qa_list.append(command_id)
    structure["quickAccessCommands"] = qa_list

    with open(structure_path, "w", encoding="utf-8") as fh:
        json.dump(structure, fh, ensure_ascii=True, indent=2)

    _invalidate_native_model_caches()
    refresh_native_commandtab(force=True, include_all_panels=False)


def _migrate_renamed_toolbars(structure: dict) -> bool:
    """
    One-time structural migration: reconcile ordered toolbar entries with orphan
    entries that arose when FreeCAD renamed built-in toolbars between versions
    (e.g. "Mesh tools" -> "Mesh Tools", "Part tools" -> "Part Tools").

    Decision logic per matched pair (ordered <-> orphan):
    - orphan has MORE commands: orphan carries fresh FreeCAD data; promote it by
      replacing the ordered entry key with the orphan's name in both the toolbars
      dict and the order array, then discard the stale ordered entry.
    - orphan has SAME or FEWER commands: ordered entry is equally good or better
      (may carry user customisations); just delete the orphan.

    Matching: first by normalised name (strips case/punctuation), then by exact
    command-set equality.

    Returns True if any workbench was modified.
    """
    dirty = False
    for _wb_name, wb_data in _coerce_dict(structure.get("workbenches", {})).items():
        toolbars = _coerce_dict(wb_data.get("toolbars", {}))
        order = toolbars.get("order")
        if not isinstance(order, list):
            continue

        order_set: set[str] = {str(x) for x in order if str(x or "").strip()}

        # Index ordered entries: norm_name -> id, id -> frozenset(cmds)
        ordered_norm_to_id: dict[str, str] = {}
        ordered_cmd_index: dict[str, frozenset] = {}
        for tb_id in list(order_set):
            tb_data = toolbars.get(tb_id)
            if not isinstance(tb_data, dict):
                continue
            cmds = frozenset(
                str(k)
                for k in _coerce_dict(tb_data.get("commands", {})).keys()
                if k != "order"
            )
            ordered_norm_to_id[_normalize_toolbar_id_for_dedup(tb_id)] = tb_id
            ordered_cmd_index[tb_id] = cmds

        # Collect actions: list of (orphan_id, matched_ordered_id)
        actions: list[tuple[str, str]] = []
        for orphan_id in list(toolbars.keys()):
            if orphan_id == "order" or orphan_id in order_set:
                continue
            if not isinstance(toolbars.get(orphan_id), dict):
                continue
            orphan_norm = _normalize_toolbar_id_for_dedup(orphan_id)
            matched: str | None = None
            # 1. Normalised name match
            if orphan_norm in ordered_norm_to_id:
                matched = ordered_norm_to_id[orphan_norm]
            # 2. Exact command set match
            if matched is None:
                orphan_cmds = frozenset(
                    str(k)
                    for k in _coerce_dict(toolbars[orphan_id].get("commands", {})).keys()
                    if k != "order"
                )
                for oid, ocmds in ordered_cmd_index.items():
                    if orphan_cmds and orphan_cmds == ocmds:
                        matched = oid
                        break
            if matched is not None:
                actions.append((orphan_id, matched))

        for orphan_id, ordered_id in actions:
            if orphan_id not in toolbars or ordered_id not in toolbars:
                continue
            orphan_cmds = frozenset(
                str(k)
                for k in _coerce_dict(toolbars[orphan_id].get("commands", {})).keys()
                if k != "order"
            )
            ordered_cmds = ordered_cmd_index.get(ordered_id, frozenset())

            if len(orphan_cmds) > len(ordered_cmds):
                # Orphan is richer (newer FreeCAD version added commands).
                # Promote orphan: update order array to use orphan name, remove
                # the stale ordered entry (orphan key already has richer data).
                del toolbars[ordered_id]
                for i, item in enumerate(order):
                    if str(item) == ordered_id:
                        order[i] = orphan_id
                        break
            else:
                # Ordered entry is equally rich or better; drop the orphan.
                del toolbars[orphan_id]
            dirty = True

    return dirty


def _load_structure() -> tuple[tuple[str, str, int], dict]:
    global _LOADED_STRUCTURE_LANGUAGE

    structure_path = _resolve_structure_path()
    structure_bytes = structure_path.read_bytes()
    structure_digest = hashlib.sha1(structure_bytes).hexdigest()
    cache_key = (str(structure_path), structure_digest, len(structure_bytes))

    if _STRUCTURE_CACHE.get("key") == cache_key:
        return cache_key, _STRUCTURE_CACHE["data"]  # type: ignore[index]

    structure = json.loads(structure_bytes.decode("utf-8"))
    _LOADED_STRUCTURE_LANGUAGE = str(structure.get("language") or "").strip()

    if _migrate_renamed_toolbars(structure):
        try:
            structure_path.write_text(
                json.dumps(structure, indent=4, ensure_ascii=False),
                encoding="utf-8",
            )
            structure_bytes = structure_path.read_bytes()
            structure_digest = hashlib.sha1(structure_bytes).hexdigest()
            cache_key = (str(structure_path), structure_digest, len(structure_bytes))
        except Exception:
            pass

    _BASE_MODEL_CACHE.clear()
    _WORKBENCH_PANEL_CACHE.clear()
    _WORKBENCH_PAYLOAD_CACHE.clear()
    _BOOTSTRAP_PAYLOAD_CACHE.clear()
    _WORKBENCH_BOOTSTRAP_CACHE.clear()
    _MODEL_PAYLOAD_CACHE.clear()
    _STRUCTURE_CACHE["key"] = cache_key
    _STRUCTURE_CACHE["data"] = structure
    return cache_key, structure


def _iter_enabled_panels(workbench_data: dict):
    toolbars = _coerce_dict(workbench_data.get("toolbars", {}))
    for panel_id, panel_data in toolbars.items():
        if panel_id == "order":
            continue
        if not isinstance(panel_data, dict):
            continue
        if bool(panel_data.get("Enabled", True)) is False:
            continue
        yield panel_id, panel_data


def _ignored_toolbar_names(structure: dict) -> set[str]:
    return {
        str(toolbar_name or "").strip().lower()
        for toolbar_name in structure.get("ignoredToolbars", [])
        if str(toolbar_name or "").strip() != ""
    }


def _is_ignored_toolbar(
    structure: dict,
    panel_id: str,
    panel_data: dict | None = None,
    workbench_name: str = "",
) -> bool:
    ignored_toolbars = _ignored_toolbar_names(structure)
    normalized_workbench = str(workbench_name or "").strip().lower()
    normalized_panel_id = str(panel_id or "").strip().lower()
    if normalized_workbench != "" and normalized_panel_id != "":
        if f"{normalized_workbench}::{normalized_panel_id}" in ignored_toolbars:
            return True
    if normalized_panel_id in ignored_toolbars:
        return True
    if isinstance(panel_data, dict):
        title = str(panel_data.get("title") or "").strip().lower()
        if normalized_workbench != "" and title != "":
            if f"{normalized_workbench}::{title}" in ignored_toolbars:
                return True
        if title in ignored_toolbars:
            return True
    return False


def _ignored_workbench_names(structure: dict) -> set[str]:
    return {
        str(workbench_name or "").strip().lower()
        for workbench_name in structure.get("ignoredWorkbenches", [])
        if str(workbench_name or "").strip() != ""
    }


def _is_ignored_workbench(structure: dict, workbench_name: str) -> bool:
    ignored_workbenches = _ignored_workbench_names(structure)
    if str(workbench_name or "").strip().lower() in ignored_workbenches:
        return True
    return _workbench_title(workbench_name).strip().lower() in ignored_workbenches


def _dropdown_button_map(structure: dict) -> dict[str, list]:
    dropdowns = structure.get("dropdownButtons", {})
    return _coerce_dict(dropdowns)


def _build_dropdown_entry(structure: dict, command_name: str, command_data: dict | None = None) -> dict:
    command_data = _coerce_dict(command_data or {})
    dropdown_items = _coerce_list(_dropdown_button_map(structure).get(command_name, []))
    menu_commands = []
    first_icon_path = ""
    for item in dropdown_items:
        if not isinstance(item, (list, tuple)) or len(item) < 1:
            continue
        dropdown_command_name = str(item[0] or "").strip()
        if dropdown_command_name == "":
            continue
        sub_entry = _build_command_entry(dropdown_command_name, {})
        if len(item) > 1:
            sub_entry["sourceWorkbenchId"] = str(item[1] or "")
        if sub_entry.get("type") != "command":
            continue
        if first_icon_path == "":
            first_icon_path = str(sub_entry.get("iconPath") or "")
        menu_commands.append(sub_entry)

    display_name = str(command_data.get("text") or "").strip()
    if display_name == "":
        display_name = str(command_name or "").replace("_ddb", "").replace("_", " ").strip()
    if display_name == "":
        display_name = command_name
    size = str(command_data.get("size") or "small").strip().lower()
    if size not in ["small", "medium", "large"]:
        size = "small"
    text_visible = _resolved_command_text_visibility(command_data, size)
    return {
        "type": "dropdown",
        "id": command_name,
        "text": display_name,
        "size": size,
        "textVisible": bool(text_visible),
        "iconPath": first_icon_path,
        "menuCommands": menu_commands,
    }


def _looks_like_technical_command_text(text: str, command_name: str) -> bool:
    normalized = str(text or "").replace("&", "").strip()
    if normalized == "":
        return False

    for candidate in NativeCommandMetadata.command_icon_candidates(command_name):
        candidate_text = str(candidate or "").replace("&", "").strip().lower()
        if normalized.lower() == candidate_text:
            return True

    if "_" not in normalized and "," not in normalized:
        return False

    if "," in normalized:
        left, _, right = normalized.partition(",")
        if left.strip() != "" and right.strip().isdigit() and " " not in left:
            return True

    return " " not in normalized


def _command_translation_contexts(command_name: str) -> list[str]:
    command_name = str(command_name or "").strip()
    if command_name == "":
        return []

    contexts: list[str] = [command_name]
    contexts.extend(_command_translation_alias_contexts(command_name))
    normalized = "".join(
        part for part in command_name.replace(",", "_").split("_") if part != ""
    )
    if normalized != "":
        if command_name.startswith("Std_"):
            contexts.append(f"StdCmd{''.join(command_name.split('_')[1:])}")
        contexts.append(f"Cmd{normalized}")
    contexts.extend(["CommandGroup", "Workbench"])

    deduplicated: list[str] = []
    for context in contexts:
        if context not in deduplicated:
            deduplicated.append(context)
    return deduplicated


def _command_translation_alias_contexts(command_name: str) -> list[str]:
    aliases = {
        "Sketcher_BSplineConvertToNURBS": ["CmdSketcherConvertToNURBS"],
        "Sketcher_BSplineDecreaseDegree": ["CmdSketcherDecreaseDegree"],
        "Sketcher_BSplineIncreaseDegree": ["CmdSketcherIncreaseDegree"],
        "Sketcher_BSplineInsertKnot": ["CmdSketcherInsertKnot"],
        "Sketcher_CompHorVer": ["CmdSketcherCompHorizontalVertical"],
        "Sketcher_RenderingOrder": ["CmdRenderingOrder", "SketcherGui::RenderingOrderAction"],
    }
    return list(aliases.get(str(command_name or "").strip(), []))


def _translated_command_text(command_name: str, source_text: str) -> str:
    cleaned_source = _normalized_visible_text(source_text)
    if cleaned_source == "":
        return ""

    for context in _command_translation_contexts(command_name):
        try:
            translated = _normalized_visible_text(App.Qt.translate(context, source_text))
        except Exception:
            translated = ""
        if translated in ["", cleaned_source]:
            translated = _macos_french_translation(context, source_text)
        if (
            translated != ""
            and translated != cleaned_source
            and _looks_like_technical_command_text(translated, command_name) is False
        ):
            return translated

    translated_label = _macos_french_command_context_label(command_name)
    if translated_label != "":
        return translated_label
    return ""


def _humanized_command_id(command_name: str) -> str:
    value = str(command_name or "").strip()
    if value == "":
        return ""
    if "," in value:
        value = value.split(",", 1)[0].strip()
    if "_" in value:
        value = value.split("_", 1)[1]
    value = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", " ", value)
    value = value.replace("_", " ").replace("-", " ")
    value = re.sub(r"\s+", " ", value).strip()
    return value or str(command_name or "").strip()


def _command_info(command_name: str) -> dict:
    cached_value = _COMMAND_INFO_CACHE.get(command_name)
    if cached_value is not None:
        return cached_value

    qt_action_metadata = _qt_action_command_metadata(command_name)
    command_info = dict(NativeCommandMetadata.command_info(command_name) or {})

    qt_text = str(qt_action_metadata.get("text") or "").strip()
    qt_tooltip = str(qt_action_metadata.get("toolTip") or "").strip()

    if qt_text != "" and _looks_like_technical_command_text(qt_text, command_name) is False:
        command_info["menuText"] = qt_text
        command_info["ActionText"] = qt_text
        command_info["DisplayText"] = qt_text
    else:
        if str(command_info.get("menuText") or "") == "":
            command_info["menuText"] = ""
        if str(command_info.get("ActionText") or "") == "":
            command_info["ActionText"] = str(command_info.get("menuText") or "")

    if qt_tooltip != "":
        command_info["toolTip"] = qt_tooltip
        if str(command_info.get("statusTip") or "").strip() == "":
            command_info["statusTip"] = qt_tooltip
    else:
        if str(command_info.get("toolTip") or "") == "":
            command_info["toolTip"] = ""
        if str(command_info.get("statusTip") or "") == "":
            command_info["statusTip"] = str(command_info.get("toolTip") or "")

    if str(command_info.get("DisplayText") or "") == "":
        command_info["DisplayText"] = str(
            command_info.get("ActionText") or command_info.get("menuText") or ""
        )

    display_text = _normalized_visible_text(str(command_info.get("DisplayText") or ""))
    translated_display_text = _translated_command_text(command_name, display_text)
    if translated_display_text != "":
        command_info["menuText"] = translated_display_text
        command_info["ActionText"] = translated_display_text
        command_info["DisplayText"] = translated_display_text
        display_text = translated_display_text

    if display_text == "" or _looks_like_technical_command_text(display_text, command_name):
        for source_text in [
            str(command_info.get("menuText") or ""),
            str(command_info.get("ActionText") or ""),
            str(command_info.get("toolTip") or ""),
        ]:
            translated_text = _translated_command_text(command_name, source_text)
            if translated_text != "":
                command_info["menuText"] = translated_text
                command_info["ActionText"] = translated_text
                command_info["DisplayText"] = translated_text
                break

    display_text = _normalized_visible_text(str(command_info.get("DisplayText") or ""))
    if display_text == "" or _looks_like_technical_command_text(display_text, command_name):
        command_info["DisplayText"] = _humanized_command_id(command_name)

    _COMMAND_INFO_CACHE[command_name] = command_info
    return command_info


def _normalized_visible_text(text: str) -> str:
    return str(text or "").replace("&", "").strip()


def _structure_language_matches_current() -> bool:
    if _LOADED_STRUCTURE_LANGUAGE == "" or _CACHE_LOCALE_SIGNATURE == "":
        return True
    return _LOADED_STRUCTURE_LANGUAGE == _CACHE_LOCALE_SIGNATURE


def _resolved_command_display_text(command_name: str, command_data: dict, command_info: dict) -> str:
    override = _COMMAND_DISPLAY_TEXT_OVERRIDES.get(str(command_name or "").strip())
    if override is not None:
        context, source_text = override
        translated = str(QCoreApplication.translate(context, source_text) or "").strip()
        return translated if translated else source_text

    resolved_text = _normalized_visible_text(
        str(
            command_info.get("DisplayText")
            or command_info.get("ActionText")
            or command_info.get("menuText")
            or command_name
        )
    )
    stored_text = _normalized_visible_text(str(command_data.get("text") or ""))
    raw_menu_text = _normalized_visible_text(str(command_info.get("menuText") or ""))
    resolved_is_technical = _looks_like_technical_command_text(resolved_text, command_name)
    stored_is_technical = _looks_like_technical_command_text(stored_text, command_name)

    translated_resolved_text = _translated_command_text(command_name, resolved_text)
    if translated_resolved_text != "":
        return translated_resolved_text

    if stored_text == "":
        return _humanized_command_id(command_name) if resolved_is_technical else resolved_text

    # If the structure was saved in a different language, stored labels are stale
    # defaults from the old locale; always use the current translation.
    if not _structure_language_matches_current():
        if resolved_text != "" and resolved_is_technical is False:
            return resolved_text
        return _humanized_command_id(command_name) if stored_is_technical else stored_text

    default_candidates = {raw_menu_text, _normalized_visible_text(command_name)}
    if stored_text in default_candidates or stored_is_technical:
        if resolved_text != "" and resolved_is_technical is False:
            return resolved_text
        return _humanized_command_id(command_name)

    return stored_text


_PANEL_TITLE_OVERRIDES: dict[str, str] = {
    # Normalised panel id -> clean display title.
    # These cover FreeCAD toolbar names that are either too verbose or had
    # inconsistent capitalisation across versions.
    "individual views": "Views",
    "individual view": "Views",
    "sketcher edit mode": "Edit Mode",
    "edit mode": "Edit Mode",
    "sketcher geometries": "Geometries",
    "sketcher constraints": "Constraints",
    "sketcher tools": "Tools",
    "sketcher b spline tools": "B-Splines",
    "sketcher b-spline tools": "B-Splines",
    "b spline tools": "B-Splines",
    "b-spline tools": "B-Splines",
    "sketcher visual": "Visual Helpers",
    "visual helpers": "Visual Helpers",
    "sketcher edit tools": "Edit Tools",
    "geometrical analysis features": "Analysis",
    "mesh tools": "Tools",
    "mesh modify": "Modify",
    "mesh boolean": "Boolean",
    "mesh cutting": "Cutting",
    "mesh segmentation": "Segmentation",
    "mesh analyze": "Analyze",
    "points tools": "Tools",
}

_WB_NAME_PREFIXES: dict[str, list[str]] = {
    "SketcherWorkbench": ["Sketcher "],
    "PartWorkbench": ["Part "],
    "PartDesignWorkbench": ["Part Design ", "PartDesign "],
    "FemWorkbench": ["FEM "],
    "TechDrawWorkbench": ["TechDraw "],
    "DraftWorkbench": ["Draft "],
    "BIMWorkbench": ["BIM ", "Arch "],
    "CAMWorkbench": ["CAM ", "Path "],
    "MeshWorkbench": ["Mesh "],
    "PointsWorkbench": ["Points "],
    "SurfaceWorkbench": ["Surface "],
    "RobotWorkbench": ["Robot "],
}


def _strip_workbench_prefix(title: str, workbench_name: str) -> str:
    """Remove the leading workbench name from a panel title when redundant."""
    prefixes = _WB_NAME_PREFIXES.get(str(workbench_name or ""), [])
    for prefix in prefixes:
        if title.lower().startswith(prefix.lower()):
            stripped = title[len(prefix):].strip()
            if stripped:
                return stripped
    return title


def _localized_panel_override(title: str) -> str:
    value = str(title or "").replace("&", "").strip()
    if value == "":
        return value

    for context in [
        "Workbench",
        "Gui::TaskView::TaskWatcherCommands",
        "MeshGui",
        "PartDesignGui",
        "SketcherGui",
        "DraftGui",
        "FreeCAD CommandTab",
    ]:
        try:
            translated = str(App.Qt.translate(context, value) or "").replace("&", "").strip()
        except Exception:
            translated = ""
        if translated in ["", value]:
            translated = _macos_french_translation(context, value)
        if translated not in ["", value]:
            return translated
    return value


def _resolved_panel_title(workbench_name: str, panel_id: str, panel_data: dict) -> str:
    title = str(panel_data.get("title") or panel_id)
    stored_title = _normalized_visible_text(title)
    panel_identifier = _normalized_visible_text(panel_id)
    try:
        StandardFunctions = _standard_functions_fallback()
        if StandardFunctions is None:
            return title
        translated_title = str(
            StandardFunctions.TranslationsMapping(workbench_name, panel_id) or panel_id
        ).replace("&", "")
        normalized_translated_title = _normalized_visible_text(translated_title)
        # If the structure was saved in a different language, ignore the stored title
        # so the current translation is used instead of a stale old-language default.
        language_matches = _structure_language_matches_current()
        if language_matches and stored_title != "" and stored_title not in [panel_identifier, normalized_translated_title]:
            return stored_title
        if translated_title not in ["", panel_id]:
            title = translated_title
    except Exception:
        pass

    # Apply known override mappings and strip redundant workbench prefixes.
    norm_key = _normalize_toolbar_id_for_dedup(title)
    override = _PANEL_TITLE_OVERRIDES.get(norm_key)
    if override:
        return _localized_panel_override(override)
    stripped = _strip_workbench_prefix(title, workbench_name)
    if stripped != title:
        return _localized_panel_override(stripped)
    return title


def _command_shortcut(command_name: str) -> str:
    try:
        cmd = Gui.Command.get(command_name)
        if cmd is None:
            return ""
        actions = cmd.getAction()
        if actions:
            shortcut = actions[0].shortcut()
            text = shortcut.toString() if hasattr(shortcut, "toString") else str(shortcut)
            return text.strip()
    except Exception:
        pass
    return ""


def _try_run_gui_command(command_name: str, *, allow_do_command: bool = True) -> bool:
    normalized = str(command_name or "").strip()
    if normalized == "":
        return False

    try:
        result = Gui.runCommand(normalized)
        if result is False:
            return False
        return True
    except Exception:
        pass

    if allow_do_command is not True:
        return False

    try:
        Gui.doCommand(normalized)
        return True
    except Exception:
        return False


def _has_gui_command(command_name: str) -> bool:
    normalized = str(command_name or "").strip()
    if normalized == "":
        return False
    try:
        return Gui.Command.get(normalized) is not None
    except Exception:
        return False


def _run_grid_toggle_command() -> bool:
    candidates = [
        "Draft_ToggleGrid",
        "Sketcher_Grid",
        "Std_ToggleGrid",
        "Std_Grid",
    ]

    for candidate in candidates:
        if _has_gui_command(candidate) is False:
            continue
        if _try_run_gui_command(candidate, allow_do_command=False):
            return True

    # Compatibility fallback for runtimes that do not expose commands via
    # Gui.Command.get but still accept runCommand by id.
    for candidate in candidates:
        if _try_run_gui_command(candidate, allow_do_command=False):
            return True

    return False


def _build_command_entry(command_name: str, command_data: dict) -> dict:
    if _is_separator_command_id(command_name):
        return {"type": "separator", "id": command_name}

    command_data = _coerce_dict(command_data or {})
    command_info = _command_info(command_name)
    text = _resolved_command_display_text(command_name, command_data, command_info)
    size = str(command_data.get("size") or "small").lower()
    icon_hint = str(command_data.get("icon") or "")
    normalized_size = size if size in ["small", "medium", "large"] else "small"
    text_visible = _resolved_command_text_visibility(command_data, normalized_size)
    menu_commands = _command_subaction_entries(command_name, command_data)
    cache_key = (
        command_name,
        text,
        normalized_size,
        icon_hint,
        str(bool(text_visible)),
        _command_subaction_signature(menu_commands),
    )

    cached_entry = _COMMAND_ENTRY_CACHE.get(cache_key)
    if cached_entry is not None:
        return _coerce_dict(cached_entry)

    shortcut = _command_shortcut(command_name)
    entry = {
        "type": "command",
        "id": command_name,
        "text": text,
        "size": normalized_size,
        "textVisible": bool(text_visible),
        "iconPath": _export_icon(command_name, icon_hint),
        "sourceWorkbenchId": str(command_data.get("sourceWorkbenchId") or ""),
        "sourceToolbarTitle": str(command_data.get("sourceToolbarTitle") or ""),
    }
    if shortcut:
        entry["shortcut"] = shortcut
    if menu_commands:
        entry["menuCommands"] = menu_commands
    _COMMAND_ENTRY_CACHE[cache_key] = entry
    return _coerce_dict(entry)


def _enrich_native_payload_menu_commands(payload: dict) -> dict:
    if isinstance(payload, dict) is False:
        return payload

    def _enrich_command(command: dict) -> None:
        if isinstance(command, dict) is False:
            return
        command_id = str(command.get("id") or "").strip()
        if command_id == "" or _is_separator_command_id(command_id):
            return
        existing_menu_commands = _coerce_list(command.get("menuCommands", []))
        if len(existing_menu_commands) == 0:
            command_data = {
                "size": str(command.get("size") or "small"),
                "textVisible": bool(command.get("textVisible", True)),
                "sourceWorkbenchId": str(command.get("sourceWorkbenchId") or ""),
                "sourceToolbarTitle": str(command.get("sourceToolbarTitle") or ""),
            }
            menu_commands = _command_subaction_entries(command_id, command_data)
            if menu_commands:
                command["menuCommands"] = menu_commands
                existing_menu_commands = menu_commands
        elif str(command.get("type") or "").strip().lower() == "command":
            _write_variant_menu_cache_entry(command_id, existing_menu_commands)
        for menu_command in existing_menu_commands:
            if isinstance(menu_command, dict):
                _enrich_command(menu_command)

    for command in _coerce_list(payload.get("quickAccessCommands", [])):
        if isinstance(command, dict):
            _enrich_command(command)

    for panel in _coerce_list(payload.get("panels", [])):
        panel_object = _coerce_dict(panel)
        for command in _coerce_list(panel_object.get("commands", [])):
            if isinstance(command, dict):
                _enrich_command(command)

    for workbench in _coerce_list(payload.get("workbenches", [])):
        workbench_object = _coerce_dict(workbench)
        for panel in _coerce_list(workbench_object.get("panels", [])):
            panel_object = _coerce_dict(panel)
            for command in _coerce_list(panel_object.get("commands", [])):
                if isinstance(command, dict):
                    _enrich_command(command)

    return payload


def _panel_title_from_identifier(panel_id: str, suffix: str = "") -> str:
    title = str(panel_id or "")
    if suffix != "" and title.endswith(suffix):
        title = title[: -len(suffix)]
    return title.replace("_", " ").strip() or panel_id


def _normalized_display_text_for_lookup(value: str) -> str:
    text = str(value or "").replace("&", " ").strip().lower()
    if text == "":
        return ""
    text = re.sub(r"[_\-]+", " ", text)
    text = re.sub(r"\s+", " ", text).strip()
    return text


def _resolve_panel_command_identifier(command_name: str, command_map: dict) -> str:
    candidate = str(command_name or "").strip()
    if candidate == "":
        return ""
    if _is_separator_command_id(candidate):
        return candidate
    if candidate in command_map:
        return candidate

    lowered_candidate = candidate.lower()
    by_lowered_key = {
        str(key).strip().lower(): str(key)
        for key in command_map.keys()
        if str(key).strip() != ""
    }
    if lowered_candidate in by_lowered_key:
        return by_lowered_key[lowered_candidate]

    lookup_token = _normalized_display_text_for_lookup(candidate)
    if lookup_token == "":
        return candidate

    text_matches = []
    for map_key, map_value in command_map.items():
        if not isinstance(map_value, dict):
            continue
        display_text = _normalized_display_text_for_lookup(str(map_value.get("text") or ""))
        if display_text == lookup_token:
            text_matches.append(str(map_key))
    if len(text_matches) == 1:
        return text_matches[0]
    return candidate


def _build_panel_command_entries(
    structure: dict,
    command_order: list,
    command_map: dict,
    source_workbench_id: str = "",
    source_toolbar_title: str = "",
) -> list[dict]:
    if (not isinstance(command_order, list) or len(command_order) == 0) and isinstance(
        command_map, dict
    ):
        command_order = list(command_map.keys())
    commands = []
    seen_command_names: set[str] = set()
    for command_name in command_order:
        command_name = _resolve_panel_command_identifier(
            str(command_name or ""),
            command_map,
        )
        if command_name == "":
            continue
        if _is_separator_command_id(command_name) is False:
            if command_name in seen_command_names:
                continue
            seen_command_names.add(command_name)
        command_data = _coerce_dict(command_map.get(command_name, {}))
        if source_workbench_id != "" and str(command_data.get("sourceWorkbenchId") or "") == "":
            command_data["sourceWorkbenchId"] = source_workbench_id
        if source_toolbar_title != "" and str(command_data.get("sourceToolbarTitle") or "") == "":
            command_data["sourceToolbarTitle"] = source_toolbar_title
        if command_name.endswith("_ddb"):
            commands.append(_build_dropdown_entry(structure, command_name, command_data))
        else:
            commands.append(_build_command_entry(command_name, command_data))
    return commands


def _build_custom_panels(structure: dict, workbench_name: str) -> tuple[list[dict], set[str]]:
    custom_root = structure.get("customToolbars", {})
    if not isinstance(custom_root, dict):
        return [], set()

    workbench_custom = custom_root.get(workbench_name, {})
    if not isinstance(workbench_custom, dict):
        return [], set()

    panels = []
    replaced_toolbars: set[str] = set()
    for panel_id, panel_payload in workbench_custom.items():
        if not isinstance(panel_payload, dict):
            continue
        command_map = _coerce_dict(panel_payload.get("commands", {}))
        command_order = list(command_map.keys())
        commands = _build_panel_command_entries(
            structure,
            command_order,
            command_map,
            source_workbench_id=str(workbench_name),
            source_toolbar_title=str(panel_id).replace("_custom", "").replace("_", " ").strip(),
        )
        if not commands:
            continue
        for original_toolbar in command_map.values():
            original_toolbar = str(original_toolbar or "").strip()
            if original_toolbar != "":
                replaced_toolbars.add(original_toolbar.lower())
        panels.append(
            {
                "id": str(panel_id),
                "title": _panel_title_from_identifier(str(panel_id), "_custom"),
                "sourceType": "custom",
                "sourceWorkbenchId": str(workbench_name),
                "commands": commands,
            }
        )
    return panels, replaced_toolbars


def _build_new_panels(structure: dict, workbench_name: str) -> list[dict]:
    new_panels_root = structure.get("newPanels", {})
    if not isinstance(new_panels_root, dict):
        return []

    panels = []
    for source_name in [workbench_name, "Global"]:
        source_panels = new_panels_root.get(source_name, {})
        if not isinstance(source_panels, dict):
            continue
        for panel_id, command_items in source_panels.items():
            command_order = []
            command_map = {}
            if isinstance(command_items, list):
                for item in command_items:
                    if not isinstance(item, (list, tuple)) or len(item) < 1:
                        continue
                    command_name = str(item[0] or "").strip()
                    if command_name == "":
                        continue
                    command_order.append(command_name)
                    command_map[command_name] = {
                        "sourceWorkbenchId": str(item[1] or source_name) if len(item) > 1 else str(source_name)
                    }
            commands = _build_panel_command_entries(
                structure,
                command_order,
                command_map,
                source_workbench_id=str(source_name),
            )
            if not commands:
                continue
            panels.append(
                {
                    "id": str(panel_id),
                    "title": _panel_title_from_identifier(str(panel_id), "_newPanel"),
                    "sourceType": "new",
                    "sourceWorkbenchId": str(source_name),
                    "commands": commands,
                }
            )
    return panels


def _ordered_panel_identifiers(workbench_data: dict) -> list[str]:
    toolbars = _coerce_dict(workbench_data.get("toolbars", {}))
    order = toolbars.get("order", [])
    if not isinstance(order, list):
        return []
    return [str(item or "") for item in order if str(item or "").strip() != ""]


def _has_native_panels(structure: dict, workbench_name: str) -> bool:
    if _is_ignored_workbench(structure, workbench_name):
        return False
    workbench_data = _coerce_dict(structure.get("workbenches", {}).get(workbench_name, {}))
    if any(True for _ in _iter_enabled_panels(workbench_data)):
        return True
    custom_panels, _replaced_toolbars = _build_custom_panels(structure, workbench_name)
    if custom_panels:
        return True
    return bool(_build_new_panels(structure, workbench_name))


def _normalize_toolbar_id_for_dedup(panel_id: str) -> str:
    value = str(panel_id or "").strip().lower()
    if value == "":
        return ""

    decomposed = unicodedata.normalize("NFD", value)
    flattened: list[str] = []
    for ch in decomposed:
        if unicodedata.category(ch).startswith("M"):
            continue
        flattened.append(ch if ch.isalnum() else " ")
    value = re.sub(r" +", " ", "".join(flattened)).strip()
    if value == "":
        return ""

    ignored_tokens = {
        "toolbar",
        "panel",
        "tool",
        "tools",
        "outil",
        "outils",
        "de",
        "des",
        "du",
        "la",
        "le",
        "les",
        "the",
        "and",
        "et",
    }
    aliases = {
        "boolean": "bool",
        "booleen": "bool",
        "booleenne": "bool",
        "booleens": "bool",
        "booleennes": "bool",
        "analyse": "analyze",
        "analysis": "analyze",
        "analyze": "analyze",
        "evaluation": "evaluate",
        "evaluer": "evaluate",
        "evalue": "evaluate",
        "evaluate": "evaluate",
        "reparer": "repair",
        "repare": "repair",
        "repair": "repair",
        "views": "view",
    }

    normalized_tokens: list[str] = []
    for token in value.split():
        if token in ignored_tokens:
            continue
        normalized_tokens.append(aliases.get(token, token))
    if normalized_tokens:
        return " ".join(normalized_tokens)
    return value


def _toolbar_command_id_set(panel: dict) -> frozenset[str]:
    return frozenset(
        str(_coerce_dict(command).get("id") or "")
        for command in _coerce_list(panel.get("commands", []))
        if str(_coerce_dict(command).get("type") or "command").strip().lower() == "command"
        and str(_coerce_dict(command).get("id") or "").strip() != ""
    )


def _toolbar_overlap_coverage(first: frozenset[str], second: frozenset[str]) -> float:
    if not first or not second:
        return 0.0
    minimum_size = min(len(first), len(second))
    if minimum_size <= 0:
        return 0.0
    intersection = len(first.intersection(second))
    if intersection <= 0:
        return 0.0
    return float(intersection) / float(minimum_size)


def _toolbar_panels_likely_duplicate(
    first_normalized_title: str,
    first_command_ids: frozenset[str],
    second_normalized_title: str,
    second_command_ids: frozenset[str],
) -> bool:
    if not first_command_ids or not second_command_ids:
        return False
    if first_command_ids == second_command_ids:
        return True

    minimum_size = min(len(first_command_ids), len(second_command_ids))
    maximum_size = max(len(first_command_ids), len(second_command_ids))
    if minimum_size < 3:
        return False
    coverage = _toolbar_overlap_coverage(first_command_ids, second_command_ids)
    symmetric_coverage = float(len(first_command_ids.intersection(second_command_ids))) / float(
        maximum_size
    )
    if coverage >= 0.95 and symmetric_coverage >= 0.85:
        return True
    if first_normalized_title != "" and first_normalized_title == second_normalized_title and coverage >= 0.85:
        return True
    return False


def _build_workbench_panels(
    structure_key: tuple[str, str, int], structure: dict, workbench_name: str
) -> list[dict]:
    cache_key = (structure_key, workbench_name)
    cached_value = _WORKBENCH_PANEL_CACHE.get(cache_key)
    if cached_value is not None:
        return copy.deepcopy(cached_value)

    workbench_data = _coerce_dict(structure.get("workbenches", {}).get(workbench_name, {}))
    panel_entries: dict[str, dict] = {}
    ordered_panel_ids = _ordered_panel_identifiers(workbench_data)
    ordered_panel_id_set = {str(panel_id) for panel_id in ordered_panel_ids if str(panel_id).strip() != ""}
    custom_panels, replaced_toolbars = _build_custom_panels(structure, workbench_name)
    new_panels = _build_new_panels(structure, workbench_name)
    new_panel_ids = {
        str(panel.get("id") or "").strip()
        for panel in new_panels
        if str(panel.get("id") or "").strip() != ""
    }

    for panel_id, panel_data in _iter_enabled_panels(workbench_data):
        if _is_ignored_toolbar(structure, panel_id, panel_data, workbench_name=str(workbench_name)):
            continue
        panel_id_normalized = str(panel_id or "").strip()
        is_orphan_toolbar = panel_id_normalized not in ordered_panel_id_set
        if is_orphan_toolbar and panel_id_normalized in new_panel_ids:
            continue
        panel_id_lower = str(panel_id or "").strip().lower()
        panel_stored_title_lower = str(panel_data.get("title") or panel_id).strip().lower()
        resolved_title = _resolved_panel_title(workbench_name, panel_id, panel_data)
        panel_live_title_lower = resolved_title.lower().strip()
        if (
            panel_id_lower in replaced_toolbars
            or panel_stored_title_lower in replaced_toolbars
            or panel_live_title_lower in replaced_toolbars
        ):
            continue

        command_order = _coerce_list(panel_data.get("order", []))
        command_map = _coerce_dict(panel_data.get("commands", {}))
        commands = _build_panel_command_entries(
            structure,
            command_order,
            command_map,
            source_workbench_id=str(workbench_name),
            source_toolbar_title=resolved_title,
        )

        if len(commands) == 0:
            continue

        panel_entries[str(panel_id)] = {
            "id": panel_id,
            "title": resolved_title,
            "sourceType": "toolbar",
            "sourceWorkbenchId": str(workbench_name),
            "commands": commands,
        }

    for panel in custom_panels + new_panels:
        panel_id = str(panel.get("id") or "").strip()
        if panel_id == "":
            continue
        if _is_ignored_toolbar(
            structure,
            panel_id,
            {"title": str(panel.get("title") or "")},
            workbench_name=str(panel.get("sourceWorkbenchId") or workbench_name),
        ):
            continue
        panel_entries[panel_id] = panel

    # Track command signatures from custom/new panels so toolbar suppression stays
    # panel-aware (avoid dropping a full toolbar because its commands are spread
    # across multiple custom/new panels).
    custom_or_new_panel_entries: list[dict[str, object]] = []
    for _panel in custom_panels + new_panels:
        _command_ids = _toolbar_command_id_set(_panel)
        if not _command_ids:
            continue
        _normalized_title = _normalize_toolbar_id_for_dedup(
            str(_panel.get("title") or _panel.get("id") or "")
        )
        custom_or_new_panel_entries.append(
            {
                "normalizedTitle": _normalized_title,
                "commandIds": _command_ids,
            }
        )

    panels = []
    seen_panel_ids = set()
    # Track accepted toolbar panels to deduplicate orphan/localized/freecad-version variants.
    accepted_toolbar_entries: list[dict[str, object]] = []

    for panel_id in ordered_panel_ids:
        if panel_id in seen_panel_ids:
            continue
        panel = panel_entries.get(panel_id)
        if panel is None:
            continue
        source_type = str(panel.get("sourceType") or "toolbar").strip().lower()
        panel_cmd_ids: frozenset[str] = frozenset()
        normalized_panel_title = ""
        if source_type == "toolbar":
            panel_cmd_ids = _toolbar_command_id_set(panel)
            normalized_panel_title = _normalize_toolbar_id_for_dedup(
                str(panel.get("title") or panel_id)
            )
            if any(
                _toolbar_panels_likely_duplicate(
                    str(_existing.get("normalizedTitle") or ""),
                    _existing.get("commandIds", frozenset()),
                    normalized_panel_title,
                    panel_cmd_ids,
                )
                for _existing in accepted_toolbar_entries
            ):
                seen_panel_ids.add(panel_id)
                continue
        # Skip toolbar panels already represented by one custom/new panel.
        if source_type == "toolbar" and panel_cmd_ids:
            if any(
                _toolbar_panels_likely_duplicate(
                    str(_existing.get("normalizedTitle") or ""),
                    _existing.get("commandIds", frozenset()),
                    normalized_panel_title,
                    panel_cmd_ids,
                )
                for _existing in custom_or_new_panel_entries
            ):
                seen_panel_ids.add(panel_id)
                continue
        panels.append(panel)
        seen_panel_ids.add(panel_id)
        if source_type == "toolbar":
            accepted_toolbar_entries.append(
                {
                    "panelId": panel_id,
                    "normalizedTitle": normalized_panel_title,
                    "commandIds": panel_cmd_ids,
                }
            )

    for panel_id, panel in panel_entries.items():
        if panel_id in seen_panel_ids:
            continue
        source_type = str(panel.get("sourceType") or "toolbar").strip().lower()
        if source_type == "toolbar":
            panel_cmd_ids = _toolbar_command_id_set(panel)
            normalized_panel_title = _normalize_toolbar_id_for_dedup(
                str(panel.get("title") or panel_id)
            )
            if any(
                _toolbar_panels_likely_duplicate(
                    str(_existing.get("normalizedTitle") or ""),
                    _existing.get("commandIds", frozenset()),
                    normalized_panel_title,
                    panel_cmd_ids,
                )
                for _existing in accepted_toolbar_entries
            ):
                continue
            # Same/near command-set dedup vs custom/new panels.
            if panel_cmd_ids and any(
                _toolbar_panels_likely_duplicate(
                    str(_existing.get("normalizedTitle") or ""),
                    _existing.get("commandIds", frozenset()),
                    normalized_panel_title,
                    panel_cmd_ids,
                )
                for _existing in custom_or_new_panel_entries
            ):
                continue
        panels.append(panel)
        if source_type == "toolbar":
            accepted_toolbar_entries.append(
                {
                    "panelId": panel_id,
                    "normalizedTitle": normalized_panel_title,
                    "commandIds": panel_cmd_ids,
                }
            )

    _WORKBENCH_PANEL_CACHE[cache_key] = copy.deepcopy(panels)
    return panels


def _build_base_model() -> tuple[tuple[str, str, int], dict, dict]:
    _ensure_runtime_icon_cache_matches_theme()
    structure_key, structure = _load_structure()
    if _BASE_MODEL_CACHE.get("key") == structure_key:
        return (
            structure_key,
            structure,
            copy.deepcopy(_BASE_MODEL_CACHE["model"]),  # type: ignore[index]
        )

    model = {
        "activeWorkbenchId": "",
        "quickAccess": [],
        "workbenches": [],
        "settings": json.loads(_native_settings_state_json()),
        "theme": CommandTabTheme.current_theme_tokens(),
    }

    for command_name in structure.get("quickAccessCommands", []):
        command_name = str(command_name or "").strip()
        if command_name == "":
            continue
        if command_name.endswith("_ddb"):
            dropdown_entry = _build_dropdown_entry(structure, command_name, {})
            text = str(dropdown_entry.get("text") or command_name)
            icon_path = str(dropdown_entry.get("iconPath") or "")
            quick_entry = dropdown_entry
            quick_entry["size"] = "small"
        else:
            qt_payload = _qt_action_command_payload(command_name, {})
            if qt_payload is not None:
                text = str(qt_payload.get("text") or "").strip()
                icon_path = str(qt_payload.get("iconPath") or "")
                if text == "":
                    command_info = _command_info(command_name)
                    text = str(
                        command_info.get("ActionText")
                        or command_info.get("DisplayText")
                        or command_info.get("menuText")
                        or command_name
                    )
            else:
                command_info = _command_info(command_name)
                text = str(
                    command_info.get("ActionText")
                    or command_info.get("DisplayText")
                    or command_info.get("menuText")
                    or command_name
                )
                icon_path = _export_icon(command_name, str(command_info.get("pixmap") or ""))
            quick_entry = {
                "type": "command",
                "id": command_name,
                "text": text,
                "size": "small",
                "iconPath": icon_path,
            }
        model["quickAccess"].append(quick_entry)

    workbenches = _coerce_dict(structure.get("workbenches", {}))
    for workbench_name, _workbench_data in workbenches.items():
        workbench_name = str(workbench_name)
        if _is_ignored_workbench(structure, workbench_name):
            continue
        if _is_available_workbench(workbench_name) is False:
            continue
        if _has_native_panels(structure, workbench_name) is False:
            continue

        model["workbenches"].append(
            {
                "id": workbench_name,
                "title": _workbench_title(workbench_name),
                "iconPath": _workbench_icon_path(workbench_name),
                "panels": [],
            }
        )

    _BASE_MODEL_CACHE["key"] = structure_key
    _BASE_MODEL_CACHE["model"] = copy.deepcopy(model)
    return structure_key, structure, model


def build_native_model() -> dict:
    structure_key, structure = _load_structure()
    metadata_cache_path = _ensure_native_metadata_cache(
        structure_key,
        structure,
        required_workbenches=None,
        include_quick_access=True,
    )
    active_workbench = _current_workbench_name()
    payload = _native_build_model_json(
        _resolve_structure_path(),
        metadata_cache_path,
        active_workbench,
        include_all_panels=False,
    )
    if payload != "":
        try:
            model = json.loads(payload)
        except Exception:
            model = None
        if isinstance(model, dict):
            model = _filter_native_payload_workbenches(model)
            model["settings"] = json.loads(_native_settings_state_json())
            return model

    structure_key, structure, model = _build_base_model()
    model["activeWorkbenchId"] = active_workbench
    model["theme"] = CommandTabTheme.current_theme_tokens()

    if active_workbench != "":
        for workbench in model.get("workbenches", []):
            if workbench.get("id") != active_workbench:
                continue
            workbench["panels"] = _build_workbench_panels(
                structure_key, structure, active_workbench
            )
            break

    return model


def _build_native_model_payload(include_all_panels: bool = False) -> tuple[str, str, set[str]]:
    structure_key, structure, model = _build_base_model()
    active_workbench = _current_workbench_name()
    theme_signature = _native_theme_signature()
    settings_signature = _native_settings_state_json()
    cache_key = (
        structure_key,
        theme_signature,
        active_workbench,
        include_all_panels,
        settings_signature,
    )
    cached_payload = _MODEL_PAYLOAD_CACHE.get(cache_key)
    if cached_payload is not None:
        loaded_workbenches = {
            workbench.get("id", "")
            for workbench in model.get("workbenches", [])
            if include_all_panels is True or workbench.get("id") == active_workbench
        }
        loaded_workbenches.discard("")
        return active_workbench, cached_payload, loaded_workbenches

    loaded_workbenches: set[str] = set()
    metadata_cache_path = _ensure_native_metadata_cache(
        structure_key,
        structure,
        required_workbenches=None
        if include_all_panels
        else ({active_workbench} if active_workbench != "" else set()),
        include_quick_access=True,
        all_workbenches_scope=bool(
            include_all_panels or _metadata_scope_all_workbench_icons_enabled()
        ),
    )
    payload = _native_build_model_json(
        _resolve_structure_path(),
        metadata_cache_path,
        active_workbench,
        include_all_panels=include_all_panels,
    )
    if payload != "":
        try:
            parsed_payload = json.loads(payload)
        except Exception:
            parsed_payload = None
        if isinstance(parsed_payload, dict):
            parsed_payload = _filter_native_payload_workbenches(parsed_payload)
            parsed_payload = _enrich_native_payload_menu_commands(parsed_payload)
            parsed_payload["settings"] = json.loads(_native_settings_state_json())
            parsed_payload.pop("themeConfig", None)
            parsed_payload["theme"] = dict(CommandTabTheme.current_theme_tokens())
            active_workbench = str(parsed_payload.get("activeWorkbenchId") or active_workbench)
            payload = json.dumps(parsed_payload, ensure_ascii=True, separators=(",", ":"))
            for workbench in parsed_payload.get("workbenches", []):
                workbench_id = str(_coerce_dict(workbench).get("id") or "")
                if workbench_id != "":
                    loaded_workbenches.add(workbench_id)
            _bounded_cache_set(_MODEL_PAYLOAD_CACHE, cache_key, payload)
            return active_workbench, payload, loaded_workbenches

    model["activeWorkbenchId"] = active_workbench
    model["theme"] = CommandTabTheme.current_theme_tokens()
    if include_all_panels is True:
        for workbench in model.get("workbenches", []):
            workbench_id = str(workbench.get("id") or "")
            if workbench_id == "":
                continue
            workbench["panels"] = _build_workbench_panels(
                structure_key, structure, workbench_id
            )
            loaded_workbenches.add(workbench_id)
    elif active_workbench != "":
        for workbench in model.get("workbenches", []):
            if workbench.get("id") != active_workbench:
                continue
            workbench["panels"] = _build_workbench_panels(
                structure_key, structure, active_workbench
            )
            loaded_workbenches.add(active_workbench)
            break

    payload = json.dumps(model, ensure_ascii=True, separators=(",", ":"))
    _bounded_cache_set(_MODEL_PAYLOAD_CACHE, cache_key, payload)
    return active_workbench, payload, loaded_workbenches


def _build_native_model_payload_safe(
    include_all_panels: bool = False,
) -> tuple[str, str, set[str], bool]:
    active_workbench, payload, loaded_workbenches = _build_native_model_payload(
        include_all_panels=include_all_panels
    )
    has_loaded_panels = False
    if include_all_panels is False:
        try:
            parsed_payload = json.loads(payload)
        except Exception:
            parsed_payload = None
        if isinstance(parsed_payload, dict):
            for workbench in _coerce_list(parsed_payload.get("workbenches", [])):
                workbench_object = _coerce_dict(workbench)
                if len(_coerce_list(workbench_object.get("panels", []))) > 0:
                    has_loaded_panels = True
                    break

    if include_all_panels is False and (
        active_workbench == ""
        or len(loaded_workbenches) == 0
        or has_loaded_panels is False
    ):
        active_workbench, payload, loaded_workbenches = _build_native_model_payload(
            include_all_panels=True
        )
        return active_workbench, payload, loaded_workbenches, True
    return active_workbench, payload, loaded_workbenches, include_all_panels


def _iter_structure_command_entries(
    structure: dict,
    workbench_names: set[str] | None = None,
    include_quick_access: bool = True,
):
    if include_quick_access is True:
        for command_name in structure.get("quickAccessCommands", []):
            command_name = str(command_name or "")
            yield command_name, {}
            if command_name.endswith("_ddb"):
                for dropdown_item in _dropdown_button_map(structure).get(command_name, []):
                    if isinstance(dropdown_item, (list, tuple)) and len(dropdown_item) >= 1:
                        yield str(dropdown_item[0] or ""), {}

    workbenches = _coerce_dict(structure.get("workbenches", {}))
    for workbench_name, workbench_data in workbenches.items():
        workbench_name = str(workbench_name)
        if _is_ignored_workbench(structure, workbench_name):
            continue
        if workbench_names is not None and workbench_name not in workbench_names:
            continue
        toolbars = _coerce_dict(workbench_data.get("toolbars", {}))
        for panel_name, panel_data in toolbars.items():
            if panel_name == "order":
                continue
            if not isinstance(panel_data, dict):
                continue
            if _is_ignored_toolbar(
                structure,
                str(panel_name),
                panel_data,
                workbench_name=workbench_name,
            ):
                continue
            commands = _coerce_dict(panel_data.get("commands", {}))
            for command_name in _coerce_list(panel_data.get("order", [])):
                command_name = str(command_name or "")
                yield command_name, _coerce_dict(commands.get(command_name, {}))
                for child_command_name in _static_variant_child_command_ids(command_name):
                    yield child_command_name, {}
                if command_name.endswith("_ddb"):
                    for dropdown_item in _dropdown_button_map(structure).get(command_name, []):
                        if isinstance(dropdown_item, (list, tuple)) and len(dropdown_item) >= 1:
                            yield str(dropdown_item[0] or ""), {}

        custom_panels, _replaced_toolbars = _build_custom_panels(structure, workbench_name)
        for panel in custom_panels + _build_new_panels(structure, workbench_name):
            for command in panel.get("commands", []):
                command_id = str(command.get("id") or "")
                if command_id != "":
                    yield command_id, {}
                    for child_command_name in _static_variant_child_command_ids(command_id):
                        yield child_command_name, {}
                if str(command.get("type") or "") == "dropdown":
                    for menu_command in command.get("menuCommands", []):
                        menu_command_id = str(menu_command.get("id") or "")
                        if menu_command_id != "":
                            yield menu_command_id, {}


def _enabled_structure_workbench_names(structure: dict) -> set[str]:
    workbench_names: set[str] = set()
    for workbench_name, _workbench_data in structure.get("workbenches", {}).items():
        workbench_name = str(workbench_name)
        normalized_workbench_name = workbench_name.strip().lower()
        if normalized_workbench_name in {"noneworkbench", "<none>", "none"}:
            continue
        normalized_title = _workbench_title(workbench_name).strip().lower()
        if normalized_title in {"<none>", "none"}:
            continue
        if _is_ignored_workbench(structure, workbench_name):
            continue
        if _is_available_workbench(workbench_name) is False:
            continue
        if _has_native_panels(structure, workbench_name):
            workbench_names.add(workbench_name)
    return workbench_names


def _native_export_structure_metadata_cache(
    structure_path: Path,
    workbench_names: set[str],
    include_quick_access: bool,
) -> dict | None:
    with StartupTrace.span(
        "bridge.export_structure_metadata_cache",
        includeQuickAccess=bool(include_quick_access),
        workbenchCount=len(workbench_names),
    ):
        if structure_path.exists() is False:
            return None

        try:
            main_window = Gui.getMainWindow()
            main_window_ptr = _resolve_cpp_pointer(main_window)
        except Exception:
            return None

        output_path = _runtime_structure_metadata_export_path()
        icon_dir = _qt_action_icons_dir()
        workbench_ids_json = json.dumps(
            sorted(str(item) for item in workbench_names if str(item).strip() != ""),
            ensure_ascii=True,
            separators=(",", ":"),
        )
        exported = False

        native_module = _load_native_extension_module()
        if native_module is not None:
            export_structure_metadata_cache = getattr(
                native_module, "export_structure_metadata_cache", None
            )
            if callable(export_structure_metadata_cache):
                try:
                    export_structure_metadata_cache(
                        main_window_ptr,
                        str(structure_path),
                        str(output_path),
                        str(icon_dir),
                        workbench_ids_json,
                        bool(include_quick_access),
                        False,
                    )
                    exported = True
                except Exception:
                    exported = False

        if exported is False:
            try:
                library = _load_native_library()
            except Exception:
                library = None
            if library is not None and hasattr(
                library, "freecad_commandtab_native_export_structure_metadata_cache"
            ):
                exported = bool(
                    library.freecad_commandtab_native_export_structure_metadata_cache(
                        main_window_ptr,
                        str(structure_path).encode("utf-8"),
                        str(output_path).encode("utf-8"),
                        str(icon_dir).encode("utf-8"),
                        workbench_ids_json.encode("utf-8"),
                        bool(include_quick_access),
                        False,
                    )
                )

        if exported is False or output_path.exists() is False:
            return None

        try:
            payload = json.loads(output_path.read_text(encoding="utf-8"))
        except Exception:
            return None
        if isinstance(payload, dict) is False:
            return None
        StartupTrace.mark(
            "bridge.export_structure_metadata_cache_ready",
            commandCount=len(payload.get("commands", {}))
            if isinstance(payload.get("commands", {}), dict)
            else 0,
            builtWorkbenchCount=len(payload.get("builtWorkbenches", []))
            if isinstance(payload.get("builtWorkbenches", []), list)
            else 0,
        )
        return payload


def _native_build_persistent_metadata_cache(
    structure_path: Path,
    cache_path: Path,
    structure_key: list,
    theme_signature: list[str],
    include_quick_access: bool,
) -> bool:
    with StartupTrace.span(
        "bridge.build_persistent_metadata_cache",
        includeQuickAccess=bool(include_quick_access),
    ):
        if structure_path.exists() is False:
            return False

        try:
            main_window = Gui.getMainWindow()
            main_window_ptr = _resolve_cpp_pointer(main_window)
        except Exception:
            return False

        cache_path.parent.mkdir(parents=True, exist_ok=True)
        icon_dir = _persistent_icon_export_dir()
        structure_key_json = json.dumps(
            list(structure_key),
            ensure_ascii=True,
            separators=(",", ":"),
        )
        theme_signature_json = json.dumps(
            list(theme_signature),
            ensure_ascii=True,
            separators=(",", ":"),
        )
        built = False

        native_module = _load_native_extension_module()
        if native_module is not None:
            build_persistent_metadata_cache = getattr(
                native_module, "build_persistent_metadata_cache", None
            )
            if callable(build_persistent_metadata_cache):
                try:
                    build_persistent_metadata_cache(
                        main_window_ptr,
                        str(structure_path),
                        str(cache_path),
                        str(icon_dir),
                        bool(include_quick_access),
                        False,
                        str(_NATIVE_METADATA_CACHE_VERSION),
                        structure_key_json,
                        theme_signature_json,
                    )
                    built = True
                except Exception:
                    built = False

        if built is False:
            try:
                library = _load_native_library()
            except Exception:
                library = None
            if library is not None and hasattr(
                library, "freecad_commandtab_native_build_persistent_metadata_cache"
            ):
                built = bool(
                    library.freecad_commandtab_native_build_persistent_metadata_cache(
                        main_window_ptr,
                        str(structure_path).encode("utf-8"),
                        str(cache_path).encode("utf-8"),
                        str(icon_dir).encode("utf-8"),
                        bool(include_quick_access),
                        False,
                        str(_NATIVE_METADATA_CACHE_VERSION).encode("utf-8"),
                        structure_key_json.encode("utf-8"),
                        theme_signature_json.encode("utf-8"),
                    )
                )

        if built is False or cache_path.exists() is False:
            return False

        StartupTrace.mark(
            "bridge.persistent_metadata_cache_ready",
            path=str(cache_path),
        )
        return True


def _load_matching_metadata_cache(
    cache_path: Path,
    expected_structure_key: list,
    theme_signature: list[str],
    environment_signature: str,
) -> dict | None:
    if cache_path.exists() is False:
        return None

    try:
        cached_payload = json.loads(cache_path.read_text(encoding="utf-8"))
    except Exception:
        return None
    if isinstance(cached_payload, dict) is False:
        return None

    cached_version = cached_payload.get("cacheVersion")
    try:
        cached_version = int(cached_version)
    except Exception:
        pass

    if (
        cached_version != _NATIVE_METADATA_CACHE_VERSION
        or cached_payload.get("structureKey") != expected_structure_key
        or cached_payload.get("themeSignature") != theme_signature
        or str(cached_payload.get("environmentSignature") or "") != environment_signature
    ):
        StartupTrace.mark(
            "bridge.native_metadata_cache_rejected",
            versionMatches=bool(cached_version == _NATIVE_METADATA_CACHE_VERSION),
            structureMatches=bool(cached_payload.get("structureKey") == expected_structure_key),
            themeMatches=bool(cached_payload.get("themeSignature") == theme_signature),
            environmentMatches=bool(
                str(cached_payload.get("environmentSignature") or "") == environment_signature
            ),
            cachedStructureKey=str(cached_payload.get("structureKey") or ""),
            expectedStructureKey=str(expected_structure_key),
            cachedEnvironmentSignature=str(cached_payload.get("environmentSignature") or ""),
            expectedEnvironmentSignature=environment_signature,
        )
        return None

    commands_payload = cached_payload.get("commands", {})
    workbench_titles = cached_payload.get("workbenchTitles", {})
    workbench_icons = cached_payload.get("workbenchIcons", {})
    panel_titles = cached_payload.get("panelTitles", {})
    built_workbenches = cached_payload.get("builtWorkbenches", [])
    quick_access_included = cached_payload.get("quickAccessIncluded", False)
    if (
        not isinstance(commands_payload, dict)
        or not isinstance(workbench_titles, dict)
        or not isinstance(workbench_icons, dict)
        or not isinstance(panel_titles, dict)
        or not isinstance(built_workbenches, list)
    ):
        return None
    return {
        "commands": commands_payload,
        "workbenchTitles": workbench_titles,
        "workbenchIcons": workbench_icons,
        "panelTitles": panel_titles,
        "builtWorkbenches": [str(item) for item in built_workbenches],
        "quickAccessIncluded": bool(quick_access_included),
        "environmentSignature": str(cached_payload.get("environmentSignature") or ""),
    }


def _coerce_metadata_cache_payload(payload) -> dict | None:
    if isinstance(payload, dict) is False:
        return None

    commands_payload = payload.get("commands", {})
    workbench_titles = payload.get("workbenchTitles", {})
    workbench_icons = payload.get("workbenchIcons", {})
    panel_titles = payload.get("panelTitles", {})
    built_workbenches = payload.get("builtWorkbenches", [])
    quick_access_included = payload.get("quickAccessIncluded", False)
    if (
        not isinstance(commands_payload, dict)
        or not isinstance(workbench_titles, dict)
        or not isinstance(workbench_icons, dict)
        or not isinstance(panel_titles, dict)
        or not isinstance(built_workbenches, list)
    ):
        return None

    return {
        "commands": commands_payload,
        "workbenchTitles": workbench_titles,
        "workbenchIcons": workbench_icons,
        "panelTitles": panel_titles,
        "builtWorkbenches": [str(item) for item in built_workbenches],
        "quickAccessIncluded": bool(quick_access_included),
        "environmentSignature": str(payload.get("environmentSignature") or ""),
    }


def _ensure_native_metadata_cache(
    structure_key: tuple[str, str, int],
    structure: dict,
    required_workbenches: set[str] | None = None,
    include_quick_access: bool = True,
    force: bool = False,
    all_workbenches_scope: bool = True,
) -> Path:
    with StartupTrace.span(
        "bridge.ensure_native_metadata_cache",
        force=bool(force),
        includeQuickAccess=bool(include_quick_access),
        requiredWorkbenchCount=0
        if required_workbenches is None
        else len(required_workbenches),
    ):
        cache_path = _persistent_metadata_cache_path()
        structure_path = _resolve_structure_path()
        theme_signature = list(_native_theme_signature())
        environment_signature = _native_metadata_environment_signature()
        expected_structure_key = [structure_key[0], structure_key[1], structure_key[2]]

        cached_payload = None
        if force is False:
            cached_payload = _load_matching_metadata_cache(
                cache_path,
                expected_structure_key,
                theme_signature,
                environment_signature,
            )

        all_enabled_workbenches = {
            str(item)
            for item in _enabled_structure_workbench_names(structure)
            if str(item).strip() != "" and _is_available_workbench(str(item))
        }
        target_workbenches = (
            set(all_enabled_workbenches)
            if required_workbenches is None
            else {
                str(item)
                for item in required_workbenches
                if str(item).strip() != "" and _is_available_workbench(str(item))
            }
        )
        scope_workbenches = (
            set(all_enabled_workbenches) if all_workbenches_scope else set(target_workbenches)
        )
        target_panels: dict[str, tuple[str, str]] = {}
        target_panel_ids: set[str] = set()
        for workbench_name in target_workbenches:
            workbench_data = _coerce_dict(structure.get("workbenches", {}).get(workbench_name, {}))
            for panel_id, panel_data in _iter_enabled_panels(workbench_data):
                target_panel_ids.add(str(panel_id))
                target_panels[str(panel_id)] = (
                    str(workbench_name),
                    str(_coerce_dict(panel_data).get("title") or panel_id),
                )
        target_command_ids: set[str] = set()
        for command_name, _command_data in _iter_structure_command_entries(
            structure,
            workbench_names=target_workbenches,
            include_quick_access=include_quick_access,
        ):
            command_name = str(command_name or "").strip()
            if command_name == "" or _is_separator_command_id(command_name):
                continue
            target_command_ids.add(command_name)

        cache_has_expected_environment_signature = False
        if isinstance(cached_payload, dict):
            cache_has_expected_environment_signature = (
                str(cached_payload.get("environmentSignature") or "")
                == environment_signature
            )

        if cached_payload is not None and force is False:
            built_workbenches = set(_coerce_list(cached_payload.get("builtWorkbenches", [])))
            quick_access_cached = bool(cached_payload.get("quickAccessIncluded", False))
            workbench_titles = _coerce_dict(cached_payload.get("workbenchTitles", {}))
            workbench_icons = _coerce_dict(cached_payload.get("workbenchIcons", {}))
            panel_titles = _coerce_dict(cached_payload.get("panelTitles", {}))
            commands_payload = _coerce_dict(cached_payload.get("commands", {}))
            missing_workbench_titles: set[str] = {
                workbench_name
                for workbench_name in scope_workbenches
                if _is_missing_or_stale_workbench_title(
                    workbench_name, str(workbench_titles.get(workbench_name) or "")
                )
            }
            missing_workbench_icons: set[str] = {
                workbench_name
                for workbench_name in scope_workbenches
                if _is_missing_or_stale_workbench_icon(
                    workbench_name, str(workbench_icons.get(workbench_name) or "")
                )
            }
            missing_panel_titles: set[str] = {
                panel_id
                for panel_id in target_panel_ids
                if str(panel_titles.get(panel_id) or "") == ""
            }
            missing_command_metadata: set[str] = {
                command_name
                for command_name in target_command_ids
                if _cached_command_metadata_is_complete(
                    _coerce_dict(commands_payload.get(command_name))
                )
                is False
            }
            if (
                target_workbenches.issubset(built_workbenches)
                and len(missing_workbench_titles) == 0
                and len(missing_workbench_icons) == 0
                and len(missing_panel_titles) == 0
                and len(missing_command_metadata) == 0
                and cache_has_expected_environment_signature is True
                and (
                    include_quick_access is False
                    or quick_access_cached is True
                )
            ):
                return cache_path
            StartupTrace.mark(
                "bridge.native_metadata_cache_miss",
                phase="pre_bootstrap",
                missingWorkbenchTitleCount=len(missing_workbench_titles),
                missingWorkbenchIconCount=len(missing_workbench_icons),
                missingPanelTitleCount=len(missing_panel_titles),
                missingCommandMetadataCount=len(missing_command_metadata),
                missingWorkbenchCount=len(target_workbenches - built_workbenches),
                quickAccessMissing=bool(include_quick_access is True and quick_access_cached is False),
                environmentMatches=bool(cache_has_expected_environment_signature),
            )

        should_build_persistent_bootstrap = (
            force is True
            or required_workbenches is None
            or _is_cpp_metadata_pipeline_enabled() is True
            or _is_persistent_metadata_bootstrap_enabled() is True
        )
        if should_build_persistent_bootstrap and _native_build_persistent_metadata_cache(
            structure_path,
            cache_path,
            expected_structure_key,
            theme_signature,
            include_quick_access=include_quick_access,
        ):
            native_seed_payload = _load_matching_metadata_cache(
                cache_path,
                expected_structure_key,
                theme_signature,
                environment_signature,
            )
            if native_seed_payload is None:
                try:
                    native_seed_payload = _coerce_metadata_cache_payload(
                        json.loads(cache_path.read_text(encoding="utf-8"))
                    )
                except Exception:
                    native_seed_payload = None
            if isinstance(native_seed_payload, dict):
                cached_payload = native_seed_payload

        if cached_payload is None:
            commands_payload: dict[str, dict[str, str]] = {}
            workbench_titles: dict[str, str] = {}
            workbench_icons: dict[str, str] = {}
            panel_titles: dict[str, str] = {}
            built_workbenches: set[str] = set()
            quick_access_cached = False
        else:
            commands_payload = _coerce_dict(cached_payload.get("commands", {}))
            workbench_titles = _coerce_dict(cached_payload.get("workbenchTitles", {}))
            workbench_icons = _coerce_dict(cached_payload.get("workbenchIcons", {}))
            panel_titles = _coerce_dict(cached_payload.get("panelTitles", {}))
            built_workbenches = set(_coerce_list(cached_payload.get("builtWorkbenches", [])))
            quick_access_cached = bool(cached_payload.get("quickAccessIncluded", False))
            cache_has_expected_environment_signature = (
                str(cached_payload.get("environmentSignature") or "")
                == environment_signature
            )

        missing_workbench_titles = {
            workbench_name
            for workbench_name in scope_workbenches
            if _is_missing_or_stale_workbench_title(
                workbench_name, str(workbench_titles.get(workbench_name) or "")
            )
        }
        missing_workbench_icons = {
            workbench_name
            for workbench_name in scope_workbenches
            if _is_missing_or_stale_workbench_icon(
                workbench_name, str(workbench_icons.get(workbench_name) or "")
            )
        }
        missing_panel_titles = {
            panel_id
            for panel_id in target_panel_ids
            if str(panel_titles.get(panel_id) or "") == ""
        }
        missing_command_metadata = {
            command_name
            for command_name in target_command_ids
            if _cached_command_metadata_is_complete(
                _coerce_dict(commands_payload.get(command_name))
            )
            is False
        }

        workbenches_to_build = (
            set(target_workbenches)
            if force is True
            else target_workbenches - built_workbenches
        )
        quick_access_to_build = include_quick_access is True and (
            force is True or quick_access_cached is False
        )

        if (
            len(missing_workbench_titles) == 0
            and len(missing_workbench_icons) == 0
            and len(missing_panel_titles) == 0
            and len(missing_command_metadata) == 0
            and len(workbenches_to_build) == 0
            and quick_access_to_build is False
            and cache_has_expected_environment_signature is True
            and force is False
        ):
            return cache_path

        # Ensure command metadata and QAction icons are resolved with the
        # corresponding workbenches loaded in FreeCAD.
        if _should_preload_workbenches_for_metadata() is True:
            _ensure_workbenches_loaded(set(target_workbenches))

        native_export_payload = _native_export_structure_metadata_cache(
            structure_path,
            workbenches_to_build,
            quick_access_to_build,
        )
        if isinstance(native_export_payload, dict):
            native_commands = native_export_payload.get("commands", {})
            if isinstance(native_commands, dict):
                for command_name, command_payload in native_commands.items():
                    if isinstance(command_payload, dict) is False:
                        continue
                    commands_payload[str(command_name)] = {
                        "text": str(command_payload.get("text") or ""),
                        "iconPath": str(command_payload.get("iconPath") or ""),
                    }

            native_titles = native_export_payload.get("workbenchTitles", {})
            if isinstance(native_titles, dict):
                for workbench_name, workbench_title in native_titles.items():
                    workbench_titles[str(workbench_name)] = str(
                        workbench_title or workbench_name
                    )

            native_icons = native_export_payload.get("workbenchIcons", {})
            if isinstance(native_icons, dict):
                for workbench_name, workbench_icon in native_icons.items():
                    workbench_icons[str(workbench_name)] = str(workbench_icon or "")

            native_panel_titles = native_export_payload.get("panelTitles", {})
            if isinstance(native_panel_titles, dict):
                for panel_id, panel_title in native_panel_titles.items():
                    panel_titles[str(panel_id)] = str(panel_title or panel_id)

            for panel_id, panel_info in target_panels.items():
                workbench_name, source_title = panel_info
                current_title = str(panel_titles.get(panel_id) or "")
                if current_title in ["", source_title, panel_id]:
                    panel_titles[panel_id] = _resolved_panel_title(
                        workbench_name,
                        panel_id,
                        {"title": source_title},
                    )

            native_built_workbenches = native_export_payload.get("builtWorkbenches", [])
            if isinstance(native_built_workbenches, list):
                built_workbenches.update(
                    str(item)
                    for item in native_built_workbenches
                    if str(item).strip() != ""
                )

            if bool(native_export_payload.get("quickAccessIncluded", False)) is True:
                quick_access_cached = True

        missing_commands: dict[str, dict] = {}
        command_metadata_workbenches = (
            target_workbenches if len(missing_command_metadata) > 0 else workbenches_to_build
        )
        command_metadata_include_quick_access = (
            include_quick_access
            if len(missing_command_metadata) > 0
            else quick_access_to_build
        )
        for command_name, command_data in _iter_structure_command_entries(
            structure,
            workbench_names=command_metadata_workbenches,
            include_quick_access=command_metadata_include_quick_access,
        ):
            if _is_separator_command_id(command_name):
                continue
            command_payload = commands_payload.get(command_name)
            if (
                isinstance(command_payload, dict)
                and _cached_command_metadata_is_complete(command_payload) is True
                and force is False
            ):
                continue
            missing_commands[command_name] = _coerce_dict(command_data)

        missing_workbench_titles = {
            workbench_name
            for workbench_name in scope_workbenches
            if _is_missing_or_stale_workbench_title(
                workbench_name, str(workbench_titles.get(workbench_name) or "")
            )
        }
        missing_workbench_icons = {
            workbench_name
            for workbench_name in scope_workbenches
            if _is_missing_or_stale_workbench_icon(
                workbench_name, str(workbench_icons.get(workbench_name) or "")
            )
        }

        if len(missing_workbench_titles) > 0 or len(missing_workbench_icons) > 0 or len(missing_commands) > 0:
            for workbench_name in missing_workbench_titles:
                workbench_titles[workbench_name] = _workbench_title(workbench_name)
            for workbench_name in missing_workbench_icons:
                workbench_icons[workbench_name] = _workbench_icon_path(
                    workbench_name, persistent=True
                )

            for command_name in list(missing_commands):
                qt_payload = _qt_action_command_payload(
                    command_name, missing_commands.get(command_name)
                )
                if qt_payload is None:
                    continue
                commands_payload[command_name] = qt_payload
                missing_commands.pop(command_name, None)

            for command_name, command_data in missing_commands.items():
                command_info = dict(_command_info(command_name) or {})
                icon_hint = str(
                    command_data.get("icon") or command_info.get("pixmap") or ""
                )
                text = _resolved_command_display_text(
                    command_name, command_data, command_info
                )
                commands_payload[command_name] = {
                    "text": text,
                    "iconPath": _export_icon(command_name, icon_hint, persistent=True),
                }

        # Refresh workbench tab icons only when missing/stale (or on forced refresh)
        # to keep startup/switch costs bounded.
        workbench_icons_to_refresh = set(missing_workbench_icons)
        if force is True:
            workbench_icons_to_refresh.update(scope_workbenches)
        for workbench_name in sorted(workbench_icons_to_refresh):
            resolved_icon = _workbench_icon_path(workbench_name, persistent=True)
            if str(resolved_icon or "").strip() != "":
                workbench_icons[workbench_name] = str(resolved_icon)

        for command_name, command_data in _iter_structure_command_entries(
            structure,
            workbench_names=target_workbenches,
            include_quick_access=include_quick_access,
        ):
            command_payload = commands_payload.get(command_name)
            if isinstance(command_payload, dict) is False:
                continue
            command_info = dict(_command_info(command_name) or {})
            command_payload["text"] = _resolved_command_display_text(
                command_name,
                command_data,
                command_info,
            )
            icon_hint = str(
                _coerce_dict(command_data).get("icon") or command_info.get("pixmap") or ""
            )
            resolved_icon_path = _export_icon(
                command_name, icon_hint, persistent=True
            )
            current_icon_path = str(command_payload.get("iconPath") or "")
            if resolved_icon_path != "" and (
                current_icon_path == ""
                or "/qt-action-icons/" in current_icon_path.replace("\\", "/")
                or _is_stable_icon_reference(current_icon_path) is False
            ):
                command_payload["iconPath"] = resolved_icon_path
            commands_payload[command_name] = command_payload

        built_workbenches.update(workbenches_to_build)
        if include_quick_access is True:
            quick_access_cached = True

        cache_path.write_text(
            json.dumps(
                {
                    "cacheVersion": _NATIVE_METADATA_CACHE_VERSION,
                    "structureKey": expected_structure_key,
                    "themeSignature": theme_signature,
                    "environmentSignature": environment_signature,
                    "workbenchTitles": workbench_titles,
                    "workbenchIcons": workbench_icons,
                    "panelTitles": panel_titles,
                    "builtWorkbenches": sorted(built_workbenches),
                    "quickAccessIncluded": bool(quick_access_cached),
                    "commands": commands_payload,
                },
                ensure_ascii=True,
                separators=(",", ":"),
            ),
            encoding="utf-8",
        )
        try:
            shutil.copy2(cache_path, _runtime_metadata_cache_path())
        except Exception:
            pass
        StartupTrace.mark(
            "bridge.native_metadata_cache_ready",
            commandCount=len(commands_payload),
            builtWorkbenchCount=len(built_workbenches),
            quickAccessIncluded=bool(quick_access_cached),
        )
        return cache_path


def build_full_native_metadata_cache(force: bool = False) -> str:
    with StartupTrace.span("bridge.build_full_native_metadata_cache", force=bool(force)):
        structure_key, structure = _load_structure()
        cache_path = _ensure_native_metadata_cache(
            structure_key,
            structure,
            required_workbenches=None,
            include_quick_access=True,
            force=force,
        )
        return str(cache_path)


def _native_build_model_json(
    structure_path: Path,
    metadata_cache_path: Path,
    active_workbench: str,
    include_all_panels: bool,
) -> str:
    theme_json = json.dumps(_native_theme_config(), ensure_ascii=True, separators=(",", ":"))
    settings_json = _native_settings_state_json()
    payload = ""

    native_module = _load_native_extension_module()
    if native_module is not None:
        build_model_json = getattr(native_module, "build_model_json", None)
        if callable(build_model_json):
            try:
                payload = str(
                    build_model_json(
                        str(structure_path),
                        str(metadata_cache_path),
                        active_workbench,
                        bool(include_all_panels),
                        theme_json,
                        settings_json,
                    )
                )
            except Exception:
                payload = ""

    if payload == "":
        try:
            library = _load_native_library()
        except Exception:
            library = None
        if (
            library is not None
            and hasattr(library, "freecad_commandtab_native_build_model_json")
            and hasattr(library, "freecad_commandtab_native_free_string")
        ):
            raw_payload = library.freecad_commandtab_native_build_model_json(
                str(structure_path).encode("utf-8"),
                str(metadata_cache_path).encode("utf-8"),
                active_workbench.encode("utf-8"),
                bool(include_all_panels),
                theme_json.encode("utf-8"),
                settings_json.encode("utf-8"),
            )
            if raw_payload:
                payload = ctypes.cast(raw_payload, ctypes.c_char_p).value.decode("utf-8")
                library.freecad_commandtab_native_free_string(raw_payload)

    return payload


def _native_build_workbench_json(
    structure_path: Path,
    metadata_cache_path: Path,
    workbench_name: str,
) -> str:
    settings_json = _native_settings_state_json()
    payload = ""

    native_module = _load_native_extension_module()
    if native_module is not None:
        build_workbench_json = getattr(native_module, "build_workbench_json", None)
        if callable(build_workbench_json):
            try:
                payload = str(
                    build_workbench_json(
                        str(structure_path),
                        str(metadata_cache_path),
                        workbench_name,
                        settings_json,
                    )
                )
            except Exception:
                payload = ""

    if payload == "":
        try:
            library = _load_native_library()
        except Exception:
            library = None
        if (
            library is not None
            and hasattr(library, "freecad_commandtab_native_build_workbench_json")
            and hasattr(library, "freecad_commandtab_native_free_string")
        ):
            raw_payload = library.freecad_commandtab_native_build_workbench_json(
                str(structure_path).encode("utf-8"),
                str(metadata_cache_path).encode("utf-8"),
                workbench_name.encode("utf-8"),
                settings_json.encode("utf-8"),
            )
            if raw_payload:
                payload = ctypes.cast(raw_payload, ctypes.c_char_p).value.decode("utf-8")
                library.freecad_commandtab_native_free_string(raw_payload)

    return payload


def _bootstrap_payload_cache_key(
    structure_key: tuple[str, str, int],
    theme_signature: tuple[str, str],
    active_workbench: str,
    include_all_panels: bool,
    settings_signature: str,
) -> dict[str, object]:
    return {
        "cacheVersion": _NATIVE_BOOTSTRAP_PAYLOAD_CACHE_VERSION,
        "metadataCacheVersion": _NATIVE_METADATA_CACHE_VERSION,
        "pipeline": "cpp-bootstrap" if _is_cpp_bootstrap_pipeline_enabled() else "python-model",
        "structureKey": [structure_key[0], structure_key[1], structure_key[2]],
        "themeSignature": [theme_signature[0], theme_signature[1]],
        "environmentSignature": _native_metadata_environment_signature(),
        "activeWorkbenchId": str(active_workbench or ""),
        "includeAllPanels": bool(include_all_panels),
        "settingsSignature": str(settings_signature or ""),
    }


def _load_cached_bootstrap_payload(expected_key: dict[str, object]) -> tuple[str, set[str]] | None:
    cache_path = _runtime_bootstrap_payload_cache_path()
    if cache_path.exists() is False:
        return None

    try:
        cached_payload = json.loads(cache_path.read_text(encoding="utf-8"))
    except Exception:
        return None
    if isinstance(cached_payload, dict) is False:
        return None

    if _coerce_dict(cached_payload.get("key")) != _coerce_dict(expected_key):
        return None

    payload = str(cached_payload.get("payload") or "")
    if payload == "":
        return None

    loaded_workbenches = {
        str(item)
        for item in _coerce_list(cached_payload.get("loadedWorkbenches", []))
        if str(item).strip() != ""
    }
    return payload, loaded_workbenches


def _write_cached_bootstrap_payload(
    cache_key: dict[str, object],
    payload: str,
    loaded_workbenches: set[str],
) -> None:
    if str(payload or "").strip() == "":
        return
    cache_path = _runtime_bootstrap_payload_cache_path()
    cache_path.parent.mkdir(parents=True, exist_ok=True)
    payload_data = {
        "key": dict(cache_key),
        "payload": str(payload),
        "loadedWorkbenches": sorted(
            str(item)
            for item in loaded_workbenches
            if str(item).strip() != ""
        ),
    }
    tmp_path = cache_path.with_suffix(".tmp")
    tmp_path.write_text(
        json.dumps(payload_data, ensure_ascii=True, separators=(",", ":")),
        encoding="utf-8",
    )
    tmp_path.replace(cache_path)


def _build_native_bootstrap_payload(
    include_all_panels: bool = False,
) -> tuple[str, str, set[str]]:
    with StartupTrace.span(
        "bridge.build_native_bootstrap_payload",
        includeAllPanels=bool(include_all_panels),
    ):
        structure_key, structure = _load_structure()
        active_workbench = _current_workbench_name()
        theme_signature = _native_theme_signature()
        settings_signature = _native_settings_state_json()
        cache_key = (
            structure_key,
            theme_signature,
            active_workbench,
            include_all_panels,
            settings_signature,
        )
        persistent_cache_key = _bootstrap_payload_cache_key(
            structure_key,
            theme_signature,
            active_workbench,
            include_all_panels,
            settings_signature,
        )

        def _loaded_workbenches_for_current_scope() -> set[str]:
            loaded: set[str] = set()
            if include_all_panels is True:
                for workbench_name, _workbench_data in structure.get("workbenches", {}).items():
                    workbench_name = str(workbench_name or "")
                    if _is_available_workbench(workbench_name) is False:
                        continue
                    if _has_native_panels(structure, workbench_name):
                        loaded.add(workbench_name)
            elif active_workbench != "" and _is_available_workbench(active_workbench):
                loaded.add(active_workbench)
            return loaded

        cached_payload = _BOOTSTRAP_PAYLOAD_CACHE.get(cache_key)
        if cached_payload is not None:
            loaded_workbenches = _loaded_workbenches_for_current_scope()
            return active_workbench, cached_payload, loaded_workbenches

        cached_payload_from_disk = _load_cached_bootstrap_payload(persistent_cache_key)
        if cached_payload_from_disk is not None:
            disk_payload, disk_loaded_workbenches = cached_payload_from_disk
            _bounded_cache_set(_BOOTSTRAP_PAYLOAD_CACHE, cache_key, disk_payload)
            if len(disk_loaded_workbenches) > 0:
                return active_workbench, disk_payload, disk_loaded_workbenches
            return active_workbench, disk_payload, _loaded_workbenches_for_current_scope()

        required_workbenches = None
        if include_all_panels is False and active_workbench != "":
            required_workbenches = {active_workbench}
        metadata_cache_path = _ensure_native_metadata_cache(
            structure_key,
            structure,
            required_workbenches=required_workbenches,
            include_quick_access=True,
            all_workbenches_scope=bool(
                include_all_panels or _metadata_scope_all_workbench_icons_enabled()
            ),
        )
        settings_json = settings_signature
        payload = ""
        native_module = _load_native_extension_module()
        if native_module is not None:
            build_bootstrap_json = getattr(native_module, "build_bootstrap_json", None)
            if callable(build_bootstrap_json):
                try:
                    payload = str(
                        build_bootstrap_json(
                            str(_resolve_structure_path()),
                            str(metadata_cache_path),
                            active_workbench,
                            bool(include_all_panels),
                            json.dumps(_native_theme_config(), ensure_ascii=True, separators=(",", ":")),
                            settings_json,
                        )
                    )
                except Exception:
                    payload = ""
        if payload == "":
            try:
                library = _load_native_library()
            except Exception:
                library = None
            if (
                library is not None
                and hasattr(library, "freecad_commandtab_native_build_bootstrap_json")
                and hasattr(library, "freecad_commandtab_native_free_string")
            ):
                raw_payload = library.freecad_commandtab_native_build_bootstrap_json(
                    str(_resolve_structure_path()).encode("utf-8"),
                    str(metadata_cache_path).encode("utf-8"),
                    active_workbench.encode("utf-8"),
                    bool(include_all_panels),
                    json.dumps(_native_theme_config(), ensure_ascii=True, separators=(",", ":")).encode("utf-8"),
                    settings_json.encode("utf-8"),
                )
                if raw_payload:
                    payload = ctypes.cast(raw_payload, ctypes.c_char_p).value.decode("utf-8")
                    library.freecad_commandtab_native_free_string(raw_payload)
        if payload == "":
            payload = json.dumps(
                {
                    "kind": "bootstrap",
                    "structurePath": str(_resolve_structure_path()),
                    "metadataCachePath": str(metadata_cache_path),
                    "activeWorkbenchId": active_workbench,
                    "includeAllPanels": include_all_panels,
                    "theme": dict(CommandTabTheme.current_theme_tokens()),
                    "settings": json.loads(settings_json),
                },
                ensure_ascii=True,
                separators=(",", ":"),
            )
        _bounded_cache_set(_BOOTSTRAP_PAYLOAD_CACHE, cache_key, payload)

        loaded_workbenches: set[str] = set()
        if include_all_panels is True:
            for workbench_name, _workbench_data in structure.get("workbenches", {}).items():
                workbench_name = str(workbench_name or "")
                if _is_available_workbench(workbench_name) is False:
                    continue
                if _has_native_panels(structure, workbench_name):
                    loaded_workbenches.add(workbench_name)
        elif active_workbench != "" and _is_available_workbench(active_workbench):
            loaded_workbenches.add(active_workbench)
        try:
            _write_cached_bootstrap_payload(
                persistent_cache_key,
                payload,
                loaded_workbenches,
            )
        except Exception:
            pass
        return active_workbench, payload, loaded_workbenches


def _build_native_workbench_payload(workbench_name: str) -> str:
    if workbench_name == "":
        raise ValueError("Workbench name is empty")

    structure_key, structure, _model = _build_base_model()
    theme_signature = _native_theme_signature()
    settings_signature = _native_settings_state_json()
    cache_key = (structure_key, theme_signature, workbench_name, settings_signature)
    cached_payload = _WORKBENCH_PAYLOAD_CACHE.get(cache_key)
    if cached_payload is not None:
        return cached_payload

    metadata_cache_path = _ensure_native_metadata_cache(
        structure_key,
        structure,
        required_workbenches={workbench_name},
        include_quick_access=False,
        all_workbenches_scope=False,
    )
    payload = _native_build_workbench_json(
        _resolve_structure_path(),
        metadata_cache_path,
        workbench_name,
    )
    if payload != "":
        try:
            parsed_payload = json.loads(payload)
        except Exception:
            parsed_payload = None
        if isinstance(parsed_payload, dict):
            parsed_payload = _enrich_native_payload_menu_commands(parsed_payload)
            payload = json.dumps(parsed_payload, ensure_ascii=True, separators=(",", ":"))
    if payload == "":
        payload = json.dumps(
            {
                "id": workbench_name,
                "title": _workbench_title(workbench_name),
                "iconPath": _workbench_icon_path(workbench_name),
                "panels": _build_workbench_panels(structure_key, structure, workbench_name),
            },
            ensure_ascii=True,
            separators=(",", ":"),
        )
    _bounded_cache_set(_WORKBENCH_PAYLOAD_CACHE, cache_key, payload)
    return payload


def _build_native_workbench_bootstrap_payload(workbench_name: str) -> str:
    if workbench_name == "":
        raise ValueError("Workbench name is empty")

    with StartupTrace.span(
        "bridge.build_native_workbench_bootstrap_payload", workbench=workbench_name
    ):
        structure_key, structure = _load_structure()
        theme_signature = _native_theme_signature()
        settings_signature = _native_settings_state_json()
        cache_key = (structure_key, theme_signature, workbench_name, settings_signature)
        cached_payload = _WORKBENCH_BOOTSTRAP_CACHE.get(cache_key)
        if cached_payload is not None:
            return cached_payload

        metadata_cache_path = _ensure_native_metadata_cache(
            structure_key,
            structure,
            required_workbenches={workbench_name},
            include_quick_access=False,
            all_workbenches_scope=False,
        )
        settings_json = settings_signature
        payload = ""
        native_module = _load_native_extension_module()
        if native_module is not None:
            build_workbench_bootstrap_json = getattr(
                native_module, "build_workbench_bootstrap_json", None
            )
            if callable(build_workbench_bootstrap_json):
                try:
                    payload = str(
                        build_workbench_bootstrap_json(
                            str(_resolve_structure_path()),
                            str(metadata_cache_path),
                            workbench_name,
                            json.dumps(_native_theme_config(), ensure_ascii=True, separators=(",", ":")),
                            settings_json,
                        )
                    )
                except Exception:
                    payload = ""
        if payload == "":
            try:
                library = _load_native_library()
            except Exception:
                library = None
            if (
                library is not None
                and hasattr(library, "freecad_commandtab_native_build_workbench_bootstrap_json")
                and hasattr(library, "freecad_commandtab_native_free_string")
            ):
                raw_payload = library.freecad_commandtab_native_build_workbench_bootstrap_json(
                    str(_resolve_structure_path()).encode("utf-8"),
                    str(metadata_cache_path).encode("utf-8"),
                    workbench_name.encode("utf-8"),
                    json.dumps(_native_theme_config(), ensure_ascii=True, separators=(",", ":")).encode("utf-8"),
                    settings_json.encode("utf-8"),
                )
                if raw_payload:
                    payload = ctypes.cast(raw_payload, ctypes.c_char_p).value.decode("utf-8")
                    library.freecad_commandtab_native_free_string(raw_payload)
        if payload == "":
            payload = json.dumps(
                {
                    "kind": "workbench_bootstrap",
                    "structurePath": str(_resolve_structure_path()),
                    "metadataCachePath": str(metadata_cache_path),
                    "workbenchId": workbench_name,
                    "theme": dict(CommandTabTheme.current_theme_tokens()),
                    "settings": json.loads(settings_json),
                },
                ensure_ascii=True,
                separators=(",", ":"),
            )
        _bounded_cache_set(_WORKBENCH_BOOTSTRAP_CACHE, cache_key, payload)
        return payload


def _decode_cached_workbench_bootstrap_state(payload: str) -> dict[str, object]:
    cached_state = _WORKBENCH_BOOTSTRAP_STATE_CACHE.get(payload)
    if isinstance(cached_state, dict):
        return cached_state
    try:
        parsed_payload = json.loads(payload)
    except Exception:
        parsed_payload = None
    if isinstance(parsed_payload, dict) is False:
        return {}
    if str(parsed_payload.get("kind") or "") != "workbench_bootstrap":
        return {}
    state = {
        "structurePath": str(parsed_payload.get("structurePath") or ""),
        "metadataCachePath": str(parsed_payload.get("metadataCachePath") or ""),
        "workbenchId": str(parsed_payload.get("workbenchId") or ""),
        "themeConfig": json.dumps(
            _coerce_dict(parsed_payload.get("themeConfig", {})),
            ensure_ascii=True,
            separators=(",", ":"),
        ),
        "settings": json.dumps(
            _coerce_dict(parsed_payload.get("settings", {})),
            ensure_ascii=True,
            separators=(",", ":"),
        ),
    }
    _WORKBENCH_BOOTSTRAP_STATE_CACHE[payload] = state
    return state


class NativeCommandTabController:
    def __init__(self):
        self._native_module = None
        self._library = None
        self._handle = None
        self._command_callback = None
        self._connected_main_window = None
        self._workbench_signal_connected = False
        self._last_model_payload = ""
        self._last_active_workbench = ""
        self._loaded_workbenches: set[str] = set()
        self._full_model_loaded = False
        self._warmup_scheduled = False
        self._warmup_queue: list[str] = []
        self._last_bootstrap_state: dict[str, object] = {}
        self._startup_repair_scheduled = False
        self._variant_menu_repair_scheduled = False
        self._variant_menu_repair_attempted = False
        self._variant_menu_repair_attempt_count = 0
        self._pending_freecad_activation_serial = 0
        self._pending_freecad_activation_workbench = ""

    def close(self) -> None:
        handle = self._handle
        self._handle = None
        self._warmup_queue = []
        self._warmup_scheduled = False
        self._startup_repair_scheduled = False
        self._variant_menu_repair_scheduled = False
        self._variant_menu_repair_attempted = False
        self._variant_menu_repair_attempt_count = 0
        self._pending_freecad_activation_serial += 1
        self._pending_freecad_activation_workbench = ""

        try:
            if self._connected_main_window is not None and self._workbench_signal_connected is True:
                self._connected_main_window.workbenchActivated.disconnect(
                    self._on_workbench_activated
                )
        except Exception:
            pass
        self._connected_main_window = None
        self._workbench_signal_connected = False

        if handle in [None, 0]:
            self._command_callback = None
            return

        try:
            if self._native_module is not None:
                destroy = getattr(self._native_module, "destroy", None)
                if callable(destroy):
                    destroy(handle)
                    return
            if self._library is not None and hasattr(
                self._library, "freecad_commandtab_native_destroy"
            ):
                self._library.freecad_commandtab_native_destroy(handle)
        except Exception:
            _logger.exception("native commandtab destroy failed")
        finally:
            self._command_callback = None

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass

    def activate(self) -> bool:
        with StartupTrace.span("bridge.native_controller.activate"):
            if self._handle:
                return True

            try:
                _ensure_dynamic_workbench_structure(_current_workbench_name())
            except Exception:
                pass

            if _is_cpp_bootstrap_pipeline_enabled() is True:
                eager_startup_panels = (
                    _is_native_eager_all_panels_on_activate()
                    or _is_native_startup_preload_all_panels_enabled()
                )
                active_workbench, payload, loaded_workbenches, full_model_loaded = (
                    _build_native_bootstrap_payload_safe(
                        include_all_panels=eager_startup_panels
                    )
                )
            else:
                active_workbench, payload, loaded_workbenches, full_model_loaded = (
                    _build_native_model_payload_safe(
                        include_all_panels=_is_native_startup_preload_all_panels_enabled()
                    )
                )

            main_window = Gui.getMainWindow()
            main_window_ptr = _resolve_cpp_pointer(main_window)

            self._native_module = _load_native_extension_module()
            if self._native_module is not None:
                create_from_json = getattr(self._native_module, "create_from_json", None)
                if callable(create_from_json):
                    handle = create_from_json(main_window_ptr, payload)
                else:
                    model_path = str(_runtime_model_path())
                    Path(model_path).write_text(payload, encoding="utf-8")
                    handle = self._native_module.create(main_window_ptr, model_path)
                self._native_module.set_command_callback(handle, self._dispatch_native_callback)
            else:
                self._library = _load_native_library()
                if hasattr(self._library, "freecad_commandtab_native_create_from_json"):
                    handle = self._library.freecad_commandtab_native_create_from_json(
                        main_window_ptr, payload.encode("utf-8")
                    )
                else:
                    model_path = str(_runtime_model_path())
                    Path(model_path).write_text(payload, encoding="utf-8")
                    handle = self._library.freecad_commandtab_native_create(
                        main_window_ptr, model_path.encode("utf-8")
                    )
                if handle in [None, 0]:
                    raise RuntimeError(_library_error(self._library))

                self._command_callback = ctypes.CFUNCTYPE(
                    None, ctypes.c_char_p, ctypes.c_void_p
                )(self._dispatch_native_callback)
                self._library.freecad_commandtab_native_set_command_callback(
                    handle,
                    ctypes.cast(self._command_callback, ctypes.c_void_p),
                    None,
                )

            self._handle = handle
            self._last_model_payload = payload
            self._last_active_workbench = active_workbench
            self._loaded_workbenches = set(loaded_workbenches)
            self._full_model_loaded = full_model_loaded
            self._variant_menu_repair_attempted = False
            self._variant_menu_repair_attempt_count = 0
            try:
                if (
                    self._connected_main_window is not None
                    and self._connected_main_window is not main_window
                    and self._workbench_signal_connected is True
                ):
                    try:
                        self._connected_main_window.workbenchActivated.disconnect(
                            self._on_workbench_activated
                        )
                    except Exception:
                        pass
                    self._workbench_signal_connected = False
                self._connected_main_window = main_window
                if self._workbench_signal_connected is False:
                    main_window.workbenchActivated.connect(self._on_workbench_activated)
                    self._workbench_signal_connected = True
            except Exception:
                self._workbench_signal_connected = False
                pass
            self._schedule_warmup()
            self._schedule_variant_menu_repair_if_needed(payload)
            if _NATIVE_STARTUP_REPAIR_ENABLED is True:
                self._schedule_startup_repair()
            StartupTrace.mark(
                "bridge.native_controller.activated",
                activeWorkbench=active_workbench,
                loadedWorkbenchCount=len(self._loaded_workbenches),
            )
            return True

    def _schedule_startup_repair(self) -> None:
        if self._startup_repair_scheduled is True:
            return
        if self._handle in [None, 0]:
            return
        self._startup_repair_scheduled = True
        StartupTrace.mark(
            "bridge.native_controller.startup_repair_scheduled",
            delayMs=int(_NATIVE_STARTUP_REPAIR_DELAY_MS),
        )
        QTimer.singleShot(_NATIVE_STARTUP_REPAIR_DELAY_MS, self._run_startup_repair)

    def _schedule_variant_menu_repair_if_needed(self, payload: str) -> None:
        if self._variant_menu_repair_attempted is True:
            return
        if self._variant_menu_repair_scheduled is True:
            return
        if self._handle in [None, 0]:
            return
        if _payload_has_command_variant_menus(payload) is True:
            self._variant_menu_repair_attempted = True
            return
        self._variant_menu_repair_scheduled = True
        StartupTrace.mark(
            "bridge.native_controller.variant_menu_repair_scheduled",
            delayMs=int(_NATIVE_VARIANT_MENU_REPAIR_DELAY_MS),
            attempt=self._variant_menu_repair_attempt_count + 1,
        )
        QTimer.singleShot(
            _NATIVE_VARIANT_MENU_REPAIR_DELAY_MS,
            self._run_variant_menu_repair,
        )

    def _run_variant_menu_repair(self) -> None:
        with StartupTrace.span("bridge.native_controller.variant_menu_repair"):
            self._variant_menu_repair_scheduled = False
            if self._handle in [None, 0]:
                return
            if self._variant_menu_repair_attempted is True:
                return
            try:
                _clear_runtime_payload_caches()
                self._variant_menu_repair_attempt_count += 1
                active_workbench, payload, loaded_workbenches, full_model_loaded = (
                    _build_native_model_payload_safe(
                        include_all_panels=_is_native_startup_preload_all_panels_enabled()
                    )
                )
                _runtime_model_path().write_text(payload, encoding="utf-8")
                self._last_model_payload = payload
                self._last_active_workbench = active_workbench
                self._loaded_workbenches = set(loaded_workbenches)
                self._full_model_loaded = full_model_loaded
                self._last_bootstrap_state = {}
                self._reload_model_payload(payload)
                if _payload_has_command_variant_menus(payload) is True:
                    self._variant_menu_repair_attempted = True
                    return
                if self._variant_menu_repair_attempt_count >= _NATIVE_VARIANT_MENU_REPAIR_MAX_ATTEMPTS:
                    self._variant_menu_repair_attempted = True
                    return
                self._schedule_variant_menu_repair_if_needed(payload)
            except Exception:
                _logger.exception("native variant menu repair failed")

    def _run_startup_repair(self) -> None:
        with StartupTrace.span("bridge.native_controller.startup_repair"):
            self._startup_repair_scheduled = False
            if self._handle in [None, 0]:
                return
            try:
                captured_count = _ensure_initialized_available_workbench_structures()
                structure_key, structure = _load_structure()
                _ensure_native_metadata_cache(
                    structure_key,
                    structure,
                    required_workbenches=None,
                    include_quick_access=True,
                    force=bool(captured_count > 0),
                )
                self.refresh(
                    force=True,
                    include_all_panels=_is_native_startup_preload_all_panels_enabled(),
                )
            except Exception:
                _logger.exception("native startup repair pass failed")

    def _schedule_warmup(self) -> None:
        if self._warmup_scheduled is True or self._full_model_loaded is True:
            return
        if _is_native_warmup_enabled() is False:
            return
        self._warmup_scheduled = True
        StartupTrace.mark(
            "bridge.native_controller.warmup_scheduled",
            delayMs=int(_NATIVE_WARMUP_DELAY_MS),
        )
        QTimer.singleShot(_NATIVE_WARMUP_DELAY_MS, self._warmup_all_workbenches)

    def _warmup_all_workbenches(self) -> None:
        with StartupTrace.span("bridge.native_controller.warmup_all_workbenches"):
            self._warmup_scheduled = False
            if self._handle in [None, 0] or self._full_model_loaded is True:
                return
            try:
                active_workbench = _current_workbench_name()
                _structure_key, structure = _load_structure()
                warmup_queue: list[str] = []
                for workbench_id, _workbench_data in structure.get("workbenches", {}).items():
                    workbench_id = str(workbench_id or "")
                    if workbench_id == "" or workbench_id in self._loaded_workbenches:
                        continue
                    if _is_ignored_workbench(structure, workbench_id):
                        continue
                    if _is_available_workbench(workbench_id) is False:
                        continue
                    if _has_native_panels(structure, workbench_id) is False:
                        continue
                    if workbench_id == active_workbench:
                        self._upsert_workbench(workbench_id, activate=True)
                        continue
                    warmup_queue.append(workbench_id)
                self._warmup_queue = warmup_queue
                StartupTrace.mark(
                    "bridge.native_controller.warmup_queue_ready",
                    queueLength=len(warmup_queue),
                )
                self._continue_warmup_queue()
            except Exception:
                pass

    def _continue_warmup_queue(self) -> None:
        if self._handle in [None, 0]:
            self._warmup_queue = []
            return
        if len(self._warmup_queue) == 0:
            self._full_model_loaded = True
            return

        workbench_id = self._warmup_queue.pop(0)
        try:
            self._upsert_workbench(workbench_id, activate=False)
        except Exception:
            pass
        QTimer.singleShot(_NATIVE_WARMUP_BATCH_DELAY_MS, self._continue_warmup_queue)

    def _write_model(self, force: bool = False, include_all_panels: bool = False) -> bool:
        if _is_cpp_bootstrap_pipeline_enabled() is True:
            active_workbench, payload, loaded_workbenches, full_model_loaded = (
                _build_native_bootstrap_payload_safe(include_all_panels=include_all_panels)
            )
        else:
            active_workbench, payload, loaded_workbenches, full_model_loaded = (
                _build_native_model_payload_safe(include_all_panels=include_all_panels)
            )
        if force is False and payload == self._last_model_payload:
            return False

        _runtime_model_path().write_text(payload, encoding="utf-8")
        self._last_model_payload = payload
        self._last_active_workbench = active_workbench
        self._loaded_workbenches = set(loaded_workbenches)
        self._full_model_loaded = full_model_loaded
        self._last_bootstrap_state = {}
        try:
            parsed_payload = json.loads(payload)
        except Exception:
            parsed_payload = None
        if isinstance(parsed_payload, dict):
            if str(parsed_payload.get("kind") or "") == "bootstrap":
                self._last_bootstrap_state = {
                    "structurePath": str(parsed_payload.get("structurePath") or ""),
                    "metadataCachePath": str(parsed_payload.get("metadataCachePath") or ""),
                    "activeWorkbenchId": str(parsed_payload.get("activeWorkbenchId") or ""),
                    "includeAllPanels": bool(parsed_payload.get("includeAllPanels", False)),
                    "themeConfig": json.dumps(
                        _coerce_dict(parsed_payload.get("themeConfig", {})),
                        ensure_ascii=True,
                        separators=(",", ":"),
                    ),
                    "settings": json.dumps(
                        _coerce_dict(parsed_payload.get("settings", {})),
                        ensure_ascii=True,
                        separators=(",", ":"),
                    ),
                }
        return True

    def _reload_model_payload(self, payload: str) -> None:
        if self._handle in [None, 0]:
            return

        if self._native_module is not None:
            reload_json = getattr(self._native_module, "reload_json", None)
            if callable(reload_json):
                reload_json(self._handle, payload)
                return
            self._native_module.reload(self._handle, str(_runtime_model_path()))
            return

        if self._library is None:
            return

        if (
            _is_cpp_bootstrap_pipeline_enabled() is True
            and isinstance(self._last_bootstrap_state, dict)
            and str(self._last_bootstrap_state.get("structurePath") or "") != ""
            and hasattr(self._library, "freecad_commandtab_native_reload_from_bootstrap")
        ):
            bootstrap = self._last_bootstrap_state
            ok = self._library.freecad_commandtab_native_reload_from_bootstrap(
                self._handle,
                str(bootstrap.get("structurePath") or "").encode("utf-8"),
                str(bootstrap.get("metadataCachePath") or "").encode("utf-8"),
                str(bootstrap.get("activeWorkbenchId") or "").encode("utf-8"),
                bool(bootstrap.get("includeAllPanels", False)),
                str(bootstrap.get("themeConfig") or "{}").encode("utf-8"),
                str(bootstrap.get("settings") or "{}").encode("utf-8"),
            )
            if ok is False:
                raise RuntimeError(_library_error(self._library))
            return

        if hasattr(self._library, "freecad_commandtab_native_reload_json"):
            ok = self._library.freecad_commandtab_native_reload_json(
                self._handle, payload.encode("utf-8")
            )
        else:
            ok = self._library.freecad_commandtab_native_reload(
                self._handle, str(_runtime_model_path()).encode("utf-8")
            )
        if ok is False:
            raise RuntimeError(_library_error(self._library))

    def _upsert_workbench(self, workbench_name: str, activate: bool = True) -> bool:
        with StartupTrace.span(
            "bridge.native_controller.upsert_workbench",
            workbench=workbench_name,
            activate=bool(activate),
        ):
            if workbench_name == "" or self._handle in [None, 0]:
                return False
            try:
                _structure_key, structure = _load_structure()
                if _is_ignored_workbench(structure, workbench_name):
                    return False
            except Exception:
                pass

            if _is_cpp_bootstrap_pipeline_enabled() is True:
                payload = _build_native_workbench_bootstrap_payload(workbench_name)
            else:
                payload = _build_native_workbench_payload(workbench_name)
            if self._native_module is not None:
                upsert_workbench_json = getattr(
                    self._native_module, "upsert_workbench_json", None
                )
                if callable(upsert_workbench_json):
                    upsert_workbench_json(self._handle, payload, activate)
                    self._loaded_workbenches.add(workbench_name)
                    if activate:
                        self._last_active_workbench = workbench_name
                    return True
                return False

            if self._library is None:
                return False

            if (
                _is_cpp_bootstrap_pipeline_enabled() is True
                and hasattr(self._library, "freecad_commandtab_native_upsert_workbench_from_bootstrap")
            ):
                bootstrap_state = _decode_cached_workbench_bootstrap_state(payload)
                if bootstrap_state:
                    ok = self._library.freecad_commandtab_native_upsert_workbench_from_bootstrap(
                        self._handle,
                        str(bootstrap_state.get("structurePath") or "").encode("utf-8"),
                        str(bootstrap_state.get("metadataCachePath") or "").encode("utf-8"),
                        str(bootstrap_state.get("workbenchId") or workbench_name).encode("utf-8"),
                        str(bootstrap_state.get("themeConfig") or "{}").encode("utf-8"),
                        str(bootstrap_state.get("settings") or "{}").encode("utf-8"),
                        bool(activate),
                    )
                    if ok:
                        self._loaded_workbenches.add(workbench_name)
                        if activate:
                            self._last_active_workbench = workbench_name
                        return True

            if not hasattr(self._library, "freecad_commandtab_native_upsert_workbench_json"):
                return False
            ok = self._library.freecad_commandtab_native_upsert_workbench_json(
                self._handle, payload.encode("utf-8"), bool(activate)
            )
            if ok:
                self._loaded_workbenches.add(workbench_name)
                if activate:
                    self._last_active_workbench = workbench_name
                return True
            raise RuntimeError(_library_error(self._library))

    def refresh(self, force: bool = False, include_all_panels: bool = False) -> None:
        if self._handle in [None, 0]:
            return

        if self._write_model(force=force, include_all_panels=include_all_panels) is False:
            return

        self._reload_model_payload(self._last_model_payload)

    def open_customization_dialog(
        self,
        workbench_name: str = "",
        panel_id: str = "",
    ) -> bool:
        if self._handle in [None, 0]:
            return False

        if self._native_module is not None:
            open_customization_dialog = getattr(
                self._native_module, "open_customization_dialog", None
            )
            if callable(open_customization_dialog):
                open_customization_dialog(self._handle, workbench_name or None, panel_id or None)
                return True
            return False

        if self._library is None or not hasattr(
            self._library, "freecad_commandtab_native_open_customization_dialog"
        ):
            return False

        ok = self._library.freecad_commandtab_native_open_customization_dialog(
            self._handle,
            (workbench_name or "").encode("utf-8"),
            (panel_id or "").encode("utf-8"),
        )
        if ok:
            return True
        raise RuntimeError(_library_error(self._library))

    def open_settings_dialog(self, settings_json: str) -> bool:
        if self._handle in [None, 0]:
            return False

        if self._native_module is not None:
            open_settings_dialog = getattr(
                self._native_module, "open_settings_dialog", None
            )
            if callable(open_settings_dialog):
                open_settings_dialog(self._handle, settings_json)
                return True
            return False

        if self._library is None or not hasattr(
            self._library, "freecad_commandtab_native_open_settings_dialog"
        ):
            return False

        ok = self._library.freecad_commandtab_native_open_settings_dialog(
            self._handle,
            settings_json.encode("utf-8"),
        )
        if ok:
            return True
        raise RuntimeError(_library_error(self._library))

    def apply_settings(self, settings_json: str) -> bool:
        if self._handle in [None, 0]:
            return False

        if self._native_module is not None:
            apply_settings_json = getattr(
                self._native_module, "apply_settings_json", None
            )
            if callable(apply_settings_json):
                apply_settings_json(self._handle, settings_json)
                return True
            return False

        if self._library is None or not hasattr(
            self._library, "freecad_commandtab_native_apply_settings_json"
        ):
            return False

        ok = self._library.freecad_commandtab_native_apply_settings_json(
            self._handle,
            settings_json.encode("utf-8"),
        )
        if ok:
            return True
        raise RuntimeError(_library_error(self._library))

    def set_ui_hide_controller_enabled(self, enabled: bool) -> bool:
        if self._handle in [None, 0]:
            return False
        if self._library is None or not hasattr(
            self._library, "freecad_commandtab_native_enable_ui_hide_controller"
        ):
            return False
        ok = self._library.freecad_commandtab_native_enable_ui_hide_controller(
            self._handle, bool(enabled)
        )
        if ok:
            return True
        raise RuntimeError(_library_error(self._library))

    def _set_active_workbench(self, workbench_name: str) -> bool:
        if workbench_name == "":
            return False

        if self._native_module is not None:
            set_active_workbench = getattr(self._native_module, "set_active_workbench", None)
            if callable(set_active_workbench):
                set_active_workbench(self._handle, workbench_name)
                self._last_active_workbench = workbench_name
                return True
            return False

        if self._library is None or not hasattr(self._library, "freecad_commandtab_native_set_active_workbench"):
            return False

        ok = self._library.freecad_commandtab_native_set_active_workbench(
            self._handle, workbench_name.encode("utf-8")
        )
        if ok:
            self._last_active_workbench = workbench_name
            return True
        return False

    def _schedule_freecad_workbench_activation(self, workbench_name: str) -> None:
        workbench_name = str(workbench_name or "").strip()
        if workbench_name == "" or workbench_name in {"NoneWorkbench"}:
            return
        if _is_available_workbench(workbench_name) is False:
            return

        self._pending_freecad_activation_serial += 1
        serial = self._pending_freecad_activation_serial
        self._pending_freecad_activation_workbench = workbench_name
        delay_ms = max(0, int(_NATIVE_WORKBENCH_ACTIVATION_DELAY_MS))
        StartupTrace.mark(
            "bridge.native_controller.freecad_activation_scheduled",
            workbench=workbench_name,
            delayMs=delay_ms,
        )

        def _activate_latest_workbench() -> None:
            if serial != self._pending_freecad_activation_serial:
                return
            target_workbench = self._pending_freecad_activation_workbench
            if target_workbench == "":
                return
            try:
                if target_workbench == _current_workbench_name():
                    return
                Gui.activateWorkbench(target_workbench)
            except Exception:
                _logger.exception("deferred FreeCAD workbench activation failed")

        QTimer.singleShot(delay_ms, _activate_latest_workbench)

    def _dispatch_native_callback(self, payload, _user_data=None) -> None:
        if payload in [None, b"", ""]:
            return

        if isinstance(payload, bytes):
            command_name = payload.decode("utf-8", errors="replace")
        else:
            command_name = str(payload)
        if command_name.startswith("__workbench__:"):
            workbench_name = command_name.split(":", 1)[1]
            if (
                workbench_name != ""
                and _is_available_workbench(workbench_name)
                and workbench_name != _current_workbench_name()
            ):
                self._schedule_freecad_workbench_activation(workbench_name)
            return

        if command_name.startswith("__commandtab_design_apply__:"):
            encoded_payload = command_name.split(":", 1)[1]
            try:
                _apply_native_design_payload(encoded_payload)
            except Exception:
                _logger.exception("commandtab_design_apply failed")
            return

        if command_name.startswith("__commandtab_design_save__:"):
            encoded_payload = command_name.split(":", 1)[1]
            try:
                _apply_native_design_payload(encoded_payload)
            except Exception:
                _logger.exception("commandtab_design_save failed")
            return

        if command_name.startswith("__commandtab_design_export__:"):
            encoded_path = command_name.split(":", 1)[1]
            try:
                _export_native_design_layout(encoded_path)
            except Exception:
                _logger.exception("commandtab_design_export failed")
            return

        if command_name.startswith("__commandtab_design_import__:"):
            encoded_path = command_name.split(":", 1)[1]
            try:
                _import_native_design_layout(encoded_path)
            except Exception:
                _logger.exception("commandtab_design_import failed")
            return

        if command_name.startswith("__commandtab_preferences_apply__:"):
            encoded_payload = command_name.split(":", 1)[1]
            try:
                _apply_native_preferences_payload(encoded_payload)
            except Exception:
                _logger.exception("commandtab_preferences_apply failed")
            return

        if command_name.startswith("__commandtab_preferences_save__:"):
            encoded_payload = command_name.split(":", 1)[1]
            try:
                _apply_native_preferences_payload(encoded_payload)
            except Exception:
                _logger.exception("commandtab_preferences_save failed")
            return

        if command_name.startswith("__commandtab_toggle_icon_text__:"):
            payload = command_name.split("__commandtab_toggle_icon_text__:", 1)[1]
            size, _, value = payload.partition(":")
            try:
                _set_native_icon_text_visibility(
                    size,
                    str(value or "").strip().lower() in ["1", "true", "yes", "on"],
                )
            except Exception:
                _logger.exception("commandtab_toggle_icon_text failed for size=%r", size)
            return

        if command_name.startswith("__commandtab_quick_access_toggle__:"):
            encoded_id = command_name.split(":", 1)[1]
            try:
                _toggle_quick_access_command(encoded_id)
            except Exception:
                _logger.exception("commandtab_quick_access_toggle failed")
            return

        if command_name == "__commandtab_design_reset__":
            try:
                _reset_native_design_layout()
            except Exception:
                _logger.exception("commandtab_design_reset failed")
            return

        if command_name == "__commandtab_design_restore__":
            try:
                _restore_native_design_layout()
            except Exception:
                _logger.exception("commandtab_design_restore failed")
            return

        if command_name == "__commandtab_design_reload__":
            try:
                _refresh_native_design_state()
            except Exception:
                _logger.exception("commandtab_design_reload failed")
            return

        if command_name == "__commandtab_design__" or command_name.startswith("__commandtab_design__:"):
            panel_id = ""
            if command_name.startswith("__commandtab_design__:"):
                panel_id = command_name.split(":", 1)[1].strip()
            try:
                if _ACTIVE_CONTROLLER is not None and _ACTIVE_CONTROLLER.open_customization_dialog(
                    "",
                    panel_id,
                ):
                    return
            except Exception:
                pass
            _logger.error(
                "native-only mode: customization dialog unavailable (controller open failed)"
            )
            return

        if command_name == "__commandtab_preferences__":
            try:
                if _ACTIVE_CONTROLLER is not None and _ACTIVE_CONTROLLER.open_settings_dialog(
                    _native_settings_state_json()
                ):
                    return
            except Exception:
                pass
            _logger.error("native-only mode: settings dialog unavailable (controller open failed)")
            return

        if command_name in ["__commandtab_toggle_grid__", "Draft_ToggleGrid"]:
            _run_grid_toggle_command()
            return

        if command_name.startswith("__commandtab_action__:"):
            encoded_payload = command_name.split(":", 1)[1]
            try:
                _trigger_encoded_command_action(encoded_payload)
            except Exception:
                _logger.exception("commandtab encoded action trigger failed")
            return

        try:
            _try_run_gui_command(command_name)
        except Exception:
            pass

    def _on_workbench_activated(self, *_args) -> None:
        with StartupTrace.span("bridge.native_controller.on_workbench_activated"):
            try:
                if _WORKBENCH_PRELOAD_IN_PROGRESS is True:
                    return
                workbench_name = _current_workbench_name()
                if workbench_name == "" or workbench_name == self._last_active_workbench:
                    return
                if workbench_name in {"NoneWorkbench"}:
                    return
                _structure_key, structure = _load_structure()
                if _is_ignored_workbench(structure, workbench_name):
                    return
                structure_changed = _ensure_dynamic_workbench_structure(workbench_name)
                StartupTrace.mark(
                    "bridge.native_controller.workbench_activated",
                    workbench=workbench_name,
                    alreadyLoaded=workbench_name in self._loaded_workbenches,
                    structureCaptured=bool(structure_changed),
                )
                if workbench_name in self._loaded_workbenches:
                    # Workbench already loaded in native shell: switch active tab
                    # directly to avoid rebuilding page content on each click/switch.
                    if self._set_active_workbench(workbench_name):
                        self._schedule_warmup()
                        return
                    if self._upsert_workbench(workbench_name, activate=True):
                        return
                if self._upsert_workbench(workbench_name, activate=True) is False:
                    self.refresh(
                        force=True,
                        include_all_panels=_is_native_startup_preload_all_panels_enabled(),
                    )
                self._schedule_warmup()
            except Exception:
                pass
