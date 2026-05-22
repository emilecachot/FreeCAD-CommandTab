from __future__ import annotations

from typing import Any


def bool_env(value: str, default: bool) -> bool:
    normalized = str(value or "").strip().lower()
    if normalized in ["1", "true", "yes", "on"]:
        return True
    if normalized in ["0", "false", "no", "off"]:
        return False
    return bool(default)


def summarize_native_startup(
    *,
    status: dict[str, Any],
    error: str,
    native_requested: bool,
    strict_native_mode: bool,
    legacy_fallback_allowed: bool,
    legacy_fallback_available: bool,
    legacy_fallback_used: bool,
) -> dict[str, Any]:
    if native_requested is False:
        return {
            "headline": "Native runtime not requested",
            "reason": "",
            "actions": [],
        }

    reason = str(error or status.get("lastError") or "").strip()
    if reason == "":
        reason = "No detailed runtime error was reported by the native backend."

    host_platform = str(status.get("hostPlatform", "unknown"))
    qt_major = str(status.get("qt_major", "unknown"))
    expected_dir = f"freecad_commandtab/native/bin/{host_platform}/qt{qt_major}"

    actions: list[str] = [
        f"Confirm native backend exists in `{expected_dir}` for this platform/Qt.",
        "Check the native status JSON in the user cache for full diagnostics.",
    ]

    # Native-only policy: Python UI fallback is no longer supported.
    # Keep the function signature stable because callers/tests still pass these
    # legacy fallback flags, but diagnostics now always guide users to native fixes.
    if strict_native_mode is True:
        actions.append(
            "Keep strict mode enabled and fix the native backend load error."
        )
    elif legacy_fallback_allowed or legacy_fallback_available or legacy_fallback_used:
        actions.append(
            "Legacy Python UI fallback is disabled in this addon version; resolve native backend availability."
        )

    headline = "Native runtime failed, CommandTab UI was not attached"

    return {"headline": headline, "reason": reason, "actions": actions}


def format_console_lines(summary: dict[str, Any]) -> list[str]:
    lines: list[str] = []
    headline = str(summary.get("headline", "")).strip()
    reason = str(summary.get("reason", "")).strip()
    actions = list(summary.get("actions", []))

    if headline != "":
        lines.append(f"FreeCAD CommandTab: {headline}")
    if reason != "":
        lines.append(f"Reason: {reason}")
    for action in actions:
        if str(action).strip() == "":
            continue
        lines.append(f"- {action}")
    return lines
