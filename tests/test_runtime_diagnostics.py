from __future__ import annotations

from freecad_commandtab.native import runtime_diagnostics


def test_summarize_native_startup_with_fallback() -> None:
    summary = runtime_diagnostics.summarize_native_startup(
        status={"hostPlatform": "linux", "qt_major": 6, "lastError": "dlopen failed"},
        error="",
        native_requested=True,
        strict_native_mode=False,
        legacy_fallback_allowed=True,
        legacy_fallback_available=True,
        legacy_fallback_used=True,
    )
    assert "not attached" in summary["headline"]
    assert "dlopen failed" in summary["reason"]
    assert any("linux/qt6" in action for action in summary["actions"])
    assert any("fallback is disabled" in action for action in summary["actions"])


def test_summarize_native_startup_strict_mode_has_action() -> None:
    summary = runtime_diagnostics.summarize_native_startup(
        status={"hostPlatform": "windows", "qt_major": 5},
        error="backend missing",
        native_requested=True,
        strict_native_mode=True,
        legacy_fallback_allowed=False,
        legacy_fallback_available=False,
        legacy_fallback_used=False,
    )
    assert "not attached" in summary["headline"]
    assert any("strict mode enabled" in action for action in summary["actions"])


def test_format_console_lines_returns_readable_lines() -> None:
    lines = runtime_diagnostics.format_console_lines(
        {
            "headline": "Native runtime failed",
            "reason": "symbol not found",
            "actions": ["Reinstall package", "Check cache status"],
        }
    )
    assert lines[0].startswith("FreeCAD CommandTab:")
    assert lines[1].startswith("Reason:")
    assert lines[2].startswith("- ")
