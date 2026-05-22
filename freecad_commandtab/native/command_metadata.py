from __future__ import annotations

import os

import FreeCAD as App
import FreeCADGui as Gui
from PySide.QtCore import QDir, QFileInfo
from PySide.QtGui import QIcon

from freecad_commandtab import paths
from freecad_commandtab.native.command_overrides import (
    SPECIAL_COMMAND_INFO_UPDATES,
)

_BUNDLED_ICON_THEME_FILE_EXTENSIONS = [".svg", ".png", ".xpm"]
_ICON_SEARCH_PATHS_SIGNATURE: tuple[str, ...] | None = None
_ICON_RELATIVE_PATH_INDEX: dict[str, str] = {}
_ICON_STEM_INDEX: dict[str, str] = {}


def _standard_functions_module():
    try:
        import Standard_Functions_CommandTab as StandardFunctions
    except Exception:
        return None
    return StandardFunctions


def normalize_command_name(command_name) -> str:
    if command_name in [None, ""]:
        return ""
    try:
        return str(command_name).strip()
    except Exception:
        return ""


def command_icon_candidates(command_name: str) -> list[str]:
    command_name = normalize_command_name(command_name)
    if command_name == "":
        return []

    candidates = [command_name]
    if command_name.endswith("_ddb"):
        candidates.append(command_name[:-4])

    if ", " in command_name:
        parent_command = command_name.split(", ")[0].strip()
        if parent_command != "":
            candidates.append(parent_command)
            if parent_command.endswith("_ddb"):
                candidates.append(parent_command[:-4])

    ordered_candidates: list[str] = []
    for candidate in candidates:
        if candidate not in ordered_candidates and candidate not in ["", None]:
            ordered_candidates.append(candidate)
    return ordered_candidates


def _external_theme_dirs() -> list[str]:
    sibling_addon_dir = os.path.abspath(
        os.path.join(paths.ROOT_DIR_STR, "..", "IconThemes")
    )
    candidate_roots = [
        os.path.join(App.getUserAppDataDir(), "Mod", "IconThemes"),
        sibling_addon_dir,
    ]

    directories: list[str] = []
    for root in candidate_roots:
        for candidate in [
            os.path.join(root, "FreeCAD-Flat-Icons-main", "icons", "Flat", "scalable"),
            os.path.join(root, "icons", "Flat", "scalable"),
            os.path.join(App.getUserAppDataDir(), "Gui", "Icons", "Flat", "scalable"),
        ]:
            if candidate not in directories and os.path.isdir(candidate):
                directories.append(candidate)
    return directories


def _resolve_icon_file_path(icon_name: str) -> str:
    icon_name = os.path.basename(normalize_command_name(icon_name))
    if icon_name == "":
        return ""

    stem = os.path.splitext(icon_name)[0]
    file_candidates: list[str] = []
    if os.path.splitext(icon_name)[1] != "":
        file_candidates.append(icon_name)
    for candidate in [stem, icon_name]:
        if candidate in ["", None]:
            continue
        for suffix in _BUNDLED_ICON_THEME_FILE_EXTENSIONS:
            file_name = f"{candidate}{suffix}"
            if file_name not in file_candidates:
                file_candidates.append(file_name)

    local_icon_dir = paths.addon_path("Resources", "icons")
    for file_name in file_candidates:
        local_path = os.path.join(local_icon_dir, file_name)
        if os.path.exists(local_path):
            return local_path

    return ""


def _current_icon_search_paths() -> list[str]:
    try:
        search_paths = list(QDir.searchPaths("icons"))
    except Exception:
        search_paths = []

    normalized_paths: list[str] = []
    for path in search_paths:
        normalized = os.path.abspath(str(path or "").strip())
        if normalized == "" or normalized in normalized_paths:
            continue
        if os.path.isdir(normalized):
            normalized_paths.append(normalized)
    return normalized_paths


def _prefer_icon_candidate(current_path: str, candidate_path: str, root_path: str) -> bool:
    if current_path == "":
        return True
    try:
        current_relative = os.path.relpath(current_path, root_path)
    except Exception:
        current_relative = current_path
    try:
        candidate_relative = os.path.relpath(candidate_path, root_path)
    except Exception:
        candidate_relative = candidate_path

    current_depth = current_relative.count(os.sep)
    candidate_depth = candidate_relative.count(os.sep)
    if candidate_depth != current_depth:
        return candidate_depth < current_depth
    return len(candidate_relative) < len(current_relative)


def _rebuild_icon_search_indexes() -> None:
    global _ICON_SEARCH_PATHS_SIGNATURE
    global _ICON_RELATIVE_PATH_INDEX
    global _ICON_STEM_INDEX

    search_paths = _current_icon_search_paths()
    signature = tuple(search_paths)
    if _ICON_SEARCH_PATHS_SIGNATURE == signature:
        return

    relative_index: dict[str, str] = {}
    stem_index: dict[str, str] = {}

    for root_path in search_paths:
        for dir_path, _dir_names, file_names in os.walk(root_path):
            for file_name in file_names:
                suffix = os.path.splitext(file_name)[1].lower()
                if suffix not in _BUNDLED_ICON_THEME_FILE_EXTENSIONS:
                    continue
                absolute_path = os.path.join(dir_path, file_name)
                try:
                    relative_path = os.path.relpath(absolute_path, root_path)
                except Exception:
                    relative_path = file_name
                relative_key = relative_path.replace("\\", "/").lower()
                existing_relative = relative_index.get(relative_key, "")
                if _prefer_icon_candidate(existing_relative, absolute_path, root_path):
                    relative_index[relative_key] = absolute_path

                stem_key = os.path.splitext(file_name)[0].lower()
                existing_stem = stem_index.get(stem_key, "")
                if _prefer_icon_candidate(existing_stem, absolute_path, root_path):
                    stem_index[stem_key] = absolute_path

    _ICON_SEARCH_PATHS_SIGNATURE = signature
    _ICON_RELATIVE_PATH_INDEX = relative_index
    _ICON_STEM_INDEX = stem_index


def _resolve_icon_from_search_paths(icon_name: str) -> str:
    icon_name = normalize_command_name(icon_name).replace("\\", "/")
    if icon_name == "":
        return ""

    _rebuild_icon_search_indexes()
    relative_name = icon_name.strip().lstrip("/")
    if relative_name == "":
        return ""

    candidates: list[str] = []
    name_root, name_extension = os.path.splitext(relative_name)
    if name_extension != "":
        candidates.append(relative_name)
    else:
        for extension in _BUNDLED_ICON_THEME_FILE_EXTENSIONS:
            candidates.append(f"{relative_name}{extension}")

    base_name = os.path.basename(relative_name)
    base_root, base_extension = os.path.splitext(base_name)
    if base_name not in ["", relative_name]:
        if base_extension != "":
            candidates.append(base_name)
        else:
            for extension in _BUNDLED_ICON_THEME_FILE_EXTENSIONS:
                candidates.append(f"{base_name}{extension}")

    for candidate in candidates:
        resolved = _ICON_RELATIVE_PATH_INDEX.get(candidate.lower(), "")
        if resolved != "":
            return resolved

    stem_candidates = []
    if name_root != "":
        stem_candidates.append(os.path.basename(name_root).lower())
    if base_root != "":
        stem_candidates.append(base_root.lower())
    for stem in stem_candidates:
        resolved = _ICON_STEM_INDEX.get(stem, "")
        if resolved != "":
            return resolved

    return ""


def _command_object(command_name: str):
    try:
        return Gui.Command.get(command_name)
    except Exception:
        return None


def _gui_icon(icon_name: str) -> QIcon:
    icon_name = normalize_command_name(icon_name)
    if icon_name == "":
        return QIcon()

    try:
        icon = Gui.getIcon(icon_name)
        if icon is not None and icon.isNull() is False:
            return icon
    except Exception:
        pass
    return QIcon()


def _command_action_icon(command_name: str) -> QIcon:
    command = _command_object(command_name)
    if command is None:
        return QIcon()

    try:
        action_list = command.getAction()
        if len(action_list) > 0:
            icon = action_list[0].icon()
            if icon is not None and icon.isNull() is False:
                return icon
    except Exception:
        pass
    return QIcon()


def has_native_override(command_name: str) -> bool:
    command_name = normalize_command_name(command_name)
    if command_name == "":
        return False
    return command_name in SPECIAL_COMMAND_INFO_UPDATES


def command_info(command_name: str) -> dict:
    command_name = normalize_command_name(command_name)
    info = {
        "menuText": "",
        "toolTip": "",
        "whatsThis": "",
        "statusTip": "",
        "pixmap": "",
        "ActionText": "",
        "name": "",
    }
    if command_name == "":
        return info

    special_updates = SPECIAL_COMMAND_INFO_UPDATES.get(command_name, {})
    if isinstance(special_updates, dict):
        for key, value in special_updates.items():
            if key in ["menuText", "toolTip", "statusTip", "ActionText", "DisplayText"]:
                continue
            info[key] = value

    StandardFunctions = _standard_functions_module()
    command = _command_object(command_name)
    if command is None:
        if str(info.get("ActionText") or "") == "":
            info["ActionText"] = str(info.get("menuText") or "")
        return info

    native_pixmap = ""
    try:
        raw_info = command.getInfo()
        if isinstance(raw_info, dict):
            info.update(raw_info)
            native_pixmap = str(raw_info.get("pixmap") or "")
    except Exception:
        pass

    if StandardFunctions is not None:
        try:
            corrected_info = StandardFunctions.CommandInfoCorrections(command_name)
            if isinstance(corrected_info, dict):
                info.update(corrected_info)
        except Exception:
            pass

    if isinstance(special_updates, dict):
        info.update(special_updates)

    # Keep the original FreeCAD command pixmap in native mode to avoid
    # addon-specific icon substitutions.
    if native_pixmap != "":
        info["pixmap"] = native_pixmap

    try:
        action_list = command.getAction()
        if len(action_list) > 0:
            info["ActionText"] = action_list[0].text()
            if str(action_list[0].toolTip() or "").strip() != "":
                info["toolTip"] = str(action_list[0].toolTip() or "")
            if str(action_list[0].statusTip() or "").strip() != "":
                info["statusTip"] = str(action_list[0].statusTip() or "")
            if len(action_list) > 1:
                if str(info.get("menuText") or "").endswith("...") is False:
                    info["menuText"] = str(info.get("menuText") or "") + "..."
                if str(info.get("ActionText") or "").endswith("...") is False:
                    info["ActionText"] = str(info.get("ActionText") or "") + "..."
    except Exception:
        pass

    if str(info.get("ActionText") or "") == "":
        info["ActionText"] = str(info.get("menuText") or "")
    if str(info.get("menuText") or "") == "":
        info["menuText"] = str(info.get("ActionText") or "")
    if isinstance(special_updates, dict):
        for text_key in ["menuText", "toolTip", "statusTip"]:
            if str(info.get(text_key) or "").strip() == "":
                value = str(special_updates.get(text_key) or "").strip()
                if value != "":
                    info[text_key] = value
    return info


def resolve_native_icon_path(command_name: str, pixmap: str = "") -> str:
    command_name = normalize_command_name(command_name)
    requested_pixmap = normalize_command_name(pixmap)
    if command_name == "" and requested_pixmap == "":
        return ""

    info = command_info(command_name) if command_name != "" else {}
    candidate_pixmaps: list[str] = []
    for candidate in [
        requested_pixmap,
        str(info.get("pixmap") or ""),
    ]:
        if candidate not in ["", None] and candidate not in candidate_pixmaps:
            candidate_pixmaps.append(candidate)

    for candidate in candidate_pixmaps:
        if candidate.startswith(":/"):
            file_info = QFileInfo(candidate)
            if file_info.exists():
                return candidate

        if candidate.startswith("icons:"):
            file_info = QFileInfo(candidate)
            if file_info.exists():
                return candidate
            candidate = candidate[len("icons:") :]

        if os.path.exists(candidate):
            return os.path.abspath(candidate)

        resolved_search_path = _resolve_icon_from_search_paths(candidate)
        if resolved_search_path != "":
            return resolved_search_path
    return ""


def load_command_icon(command_name: str, pixmap: str = "") -> QIcon:
    action_icon = _command_action_icon(command_name)
    if action_icon.isNull() is False:
        return action_icon

    resolved_path = resolve_native_icon_path(command_name, pixmap)
    if resolved_path != "":
        icon = QIcon(resolved_path)
        if icon.isNull() is False:
            return icon

    info = command_info(command_name)
    for candidate in [pixmap, str(info.get("pixmap") or "")]:
        candidate = normalize_command_name(candidate)
        if candidate == "":
            continue
        icon = _gui_icon(candidate)
        if icon.isNull() is False:
            return icon

    for candidate in command_icon_candidates(command_name):
        icon = _gui_icon(candidate)
        if icon.isNull() is False:
            return icon
    return QIcon()
