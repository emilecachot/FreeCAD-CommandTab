from __future__ import annotations

import json

from freecad_commandtab import paths


def _load_overrides_payload() -> dict:
    override_path = paths.addon_path("freecad_commandtab", "native", "command_overrides.json")
    try:
        with open(override_path, encoding="utf-8") as handle:
            payload = json.load(handle)
    except Exception:
        payload = {}
    return payload if isinstance(payload, dict) else {}


_PAYLOAD = _load_overrides_payload()

SPECIAL_COMMAND_INFO_UPDATES = dict(_PAYLOAD.get("specialCommandInfoUpdates", {}))

SPECIAL_MENU_TEXT = {
    command_name: command_info["menuText"]
    for command_name, command_info in SPECIAL_COMMAND_INFO_UPDATES.items()
    if isinstance(command_info, dict) and "menuText" in command_info
}

SPECIAL_PIXMAPS = dict(_PAYLOAD.get("specialPixmaps", {}))

CUSTOM_COMMAND_PIXMAP_FILES = dict(_PAYLOAD.get("customCommandPixmapFiles", {}))
