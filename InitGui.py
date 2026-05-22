# *************************************************************************
# *                                                                       *
# * Copyright (c) 2019-2024 Hakan Seven, Geolta, Paul Ebbers              *
# *                                                                       *
# * This program is free software; you can redistribute it and/or modify  *
# * it under the terms of the GNU Lesser General Public License (LGPL)    *
# * as published by the Free Software Foundation; either version 3 of     *
# * the License, or (at your option) any later version.                   *
# * for detail see the LICENCE text file.                                 *
# *                                                                       *
# * This program is distributed in the hope that it will be useful,       *
# * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# * GNU Library General Public License for more details.                  *
# *                                                                       *
# * You should have received a copy of the GNU Library General Public     *
# * License along with this program; if not, write to the Free Software   *
# * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# * USA                                                                   *
# *                                                                       *
# *************************************************************************
from __future__ import annotations

import os
import traceback
from contextlib import contextmanager

import FreeCAD as App

try:
    from PySide.QtCore import QTimer
except Exception:
    QTimer = None

from freecad_commandtab import gui_compat as GuiCompat
from freecad_commandtab.native import bootstrap as NativeBootstrap

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


CurrentStructureVersion = 2

# FreeCAD 1.1 console/snap variants may not expose Gui.getMainWindow.
# Install a compatibility shim before importing/initializing UI layers.
GuiCompat.ensure_get_main_window()


def _defer_native_attach_enabled() -> bool:
    value = str(
        os.environ.get("FREECAD_COMMANDTAB_DEFER_NATIVE_ATTACH", "0") or "0"
    ).strip().lower()
    return value not in {"0", "false", "no", "off"}


def _defer_native_attach_delay_ms() -> int:
    try:
        return max(
            0,
            int(
                str(
                    os.environ.get("FREECAD_COMMANDTAB_DEFER_NATIVE_ATTACH_DELAY_MS", "120")
                    or "120"
                ).strip()
            ),
        )
    except Exception:
        return 120


def _startup_attach_retry_max() -> int:
    try:
        return max(
            0,
            int(
                str(
                    os.environ.get("FREECAD_COMMANDTAB_STARTUP_ATTACH_RETRY_MAX", "20")
                    or "20"
                ).strip()
            ),
        )
    except Exception:
        return 20


def _startup_attach_retry_delay_ms() -> int:
    try:
        return max(
            40,
            int(
                str(
                    os.environ.get("FREECAD_COMMANDTAB_STARTUP_ATTACH_RETRY_DELAY_MS", "140")
                    or "140"
                ).strip()
            ),
        )
    except Exception:
        return 140


def _post_startup_delay_ms() -> int:
    try:
        return max(
            0,
            int(
                str(
                    os.environ.get("FREECAD_COMMANDTAB_POST_STARTUP_DELAY_MS", "160")
                    or "160"
                ).strip()
            ),
        )
    except Exception:
        return 160


def QT_TRANSLATE_NOOP(context, text):
    return text


def _safe_commandtab_startup(
    _NativeBootstrap=NativeBootstrap,
    _StartupTrace=StartupTrace,
    _CurrentStructureVersion=CurrentStructureVersion,
    _App=App,
    _QTimer=QTimer,
    _defer_native_attach_enabled_fn=None,
    _defer_native_attach_delay_ms_fn=None,
    _startup_attach_retry_max_fn=None,
    _startup_attach_retry_delay_ms_fn=None,
    _post_startup_delay_ms_fn=None,
) -> None:
    if _defer_native_attach_enabled_fn is None:
        _defer_native_attach_enabled_fn = globals().get("_defer_native_attach_enabled")
    if _defer_native_attach_delay_ms_fn is None:
        _defer_native_attach_delay_ms_fn = globals().get("_defer_native_attach_delay_ms")
    if _startup_attach_retry_max_fn is None:
        _startup_attach_retry_max_fn = globals().get("_startup_attach_retry_max")
    if _startup_attach_retry_delay_ms_fn is None:
        _startup_attach_retry_delay_ms_fn = globals().get("_startup_attach_retry_delay_ms")
    if _post_startup_delay_ms_fn is None:
        _post_startup_delay_ms_fn = globals().get("_post_startup_delay_ms")

    NativeCommandTabActive = False
    startup_attach_state = {
        "completed": False,
        "running": False,
        "attempts": 0,
    }
    post_startup_state = {
        "scheduled": False,
        "ran": False,
    }
    _StartupTrace.reset_session("InitGui")
    _StartupTrace.mark("InitGui.start", structureVersion=_CurrentStructureVersion)
    try:
        _NativeBootstrap.reset_visibility_debug_log()
    except Exception:
        pass

    def _run_startup_step(label: str, callback, *args, **kwargs) -> None:
        try:
            with _StartupTrace.span(label):
                callback(*args, **kwargs)
        except Exception:
            startup_error = traceback.format_exc()
            try:
                _App.Console.PrintError(
                    "FreeCAD CommandTab startup step failed (continuing):\n"
                    + startup_error
                    + "\n"
                )
            except Exception:
                print(startup_error)

    def _finish_startup_attach() -> None:
        nonlocal NativeCommandTabActive
        if startup_attach_state["completed"] is True:
            return
        if startup_attach_state["running"] is True:
            return
        startup_attach_state["running"] = True
        startup_attach_state["attempts"] += 1
        attach_success = False
        with _StartupTrace.span("InitGui.finish_startup_attach"):
            try:
                with _StartupTrace.span("InitGui.activate_native_if_requested"):
                    NativeCommandTabActive = _NativeBootstrap.activate_native_if_requested()
                try:
                    with _StartupTrace.span(
                        "InitGui.attach_ui", nativeCommandTabActive=bool(NativeCommandTabActive)
                    ):
                        _NativeBootstrap.attach_ui(NativeCommandTabActive)
                    attach_success = True
                    startup_attach_state["completed"] = True
                except Exception as e:
                    import Parameters_CommandTab

                    startup_window_not_ready = False
                    try:
                        startup_window_not_ready = bool(
                            _NativeBootstrap.is_main_window_not_ready_error(e)
                        )
                    except Exception:
                        startup_window_not_ready = False

                    if startup_window_not_ready is True:
                        _StartupTrace.mark(
                            "InitGui.attach_ui_wait_main_window",
                            attempt=int(startup_attach_state["attempts"]),
                            reason=str(e),
                        )
                    else:
                        startup_error = traceback.format_exc()
                        try:
                            _App.Console.PrintError(
                                "FreeCAD CommandTab startup failed in InitGui.attach_ui:\n"
                                + startup_error
                                + "\n"
                            )
                        except Exception:
                            print(startup_error)

                    if bool(getattr(Parameters_CommandTab, "DEBUG_MODE", False)) is True:
                        print(f"{e.with_traceback(e.__traceback__)}, 0")

                with _StartupTrace.span(
                    "InitGui.schedule_productivity_layer",
                    nativeCommandTabActive=bool(NativeCommandTabActive),
                ):
                    _NativeBootstrap.schedule_productivity_layer(NativeCommandTabActive)
                with _StartupTrace.span("InitGui.ensure_commandtab_visible"):
                    _NativeBootstrap.ensure_commandtab_visible_deferred()
                if attach_success is True:
                    _StartupTrace.mark(
                        "InitGui.ready", nativeCommandTabActive=bool(NativeCommandTabActive)
                    )
            except Exception:
                startup_error = traceback.format_exc()
                try:
                    _App.Console.PrintError(
                        "FreeCAD CommandTab startup failed (guarded):\n"
                        + startup_error
                        + "\n"
                    )
                except Exception:
                    print(startup_error)
            finally:
                startup_attach_state["running"] = False
                if startup_attach_state["completed"] is not True:
                    if callable(_startup_attach_retry_max_fn):
                        max_retries = int(_startup_attach_retry_max_fn())
                    else:
                        max_retries = 20
                    if (
                        _QTimer is not None
                        and startup_attach_state["attempts"] <= max_retries
                    ):
                        if callable(_startup_attach_retry_delay_ms_fn):
                            retry_delay = int(_startup_attach_retry_delay_ms_fn())
                        else:
                            retry_delay = 140
                        _StartupTrace.mark(
                            "InitGui.retry_startup_attach",
                            attempt=int(startup_attach_state["attempts"]),
                            maxRetry=int(max_retries),
                            delayMs=int(retry_delay),
                        )
                        _QTimer.singleShot(int(retry_delay), _finish_startup_attach)

    def _run_post_startup_steps() -> None:
        if post_startup_state["ran"] is True:
            return
        post_startup_state["ran"] = True
        _run_startup_step("InitGui.apply_theme_preferences", _NativeBootstrap.apply_theme_preferences)
        _run_startup_step(
            "InitGui.maybe_activate_bundled_icon_theme",
            _NativeBootstrap.maybe_activate_bundled_icon_theme,
        )
        _run_startup_step("InitGui.remove_test_workbench", _NativeBootstrap.remove_test_workbench)
        _run_startup_step(
            "InitGui.configure_overlay_preferences",
            _NativeBootstrap.configure_overlay_preferences,
        )

    def _schedule_post_startup_steps() -> None:
        if post_startup_state["scheduled"] is True:
            return
        post_startup_state["scheduled"] = True
        if callable(_post_startup_delay_ms_fn):
            delay_ms = int(_post_startup_delay_ms_fn())
        else:
            delay_ms = 160
        _StartupTrace.mark("InitGui.defer_post_startup_steps", delayMs=int(delay_ms))
        if _QTimer is None or delay_ms <= 0:
            _run_post_startup_steps()
            return
        _QTimer.singleShot(int(delay_ms), _run_post_startup_steps)

    try:
        _run_startup_step("InitGui.configure_logging", _NativeBootstrap.configure_logging)
        _run_startup_step("InitGui.register_translations", _NativeBootstrap.register_translations)
        _run_startup_step(
            "InitGui.ensure_structure_files",
            _NativeBootstrap.ensure_structure_files,
            _CurrentStructureVersion,
        )
        _schedule_post_startup_steps()
        if callable(_defer_native_attach_enabled_fn):
            defer_attach_enabled = bool(_defer_native_attach_enabled_fn())
        else:
            env_value = str(
                os.environ.get("FREECAD_COMMANDTAB_DEFER_NATIVE_ATTACH", "0") or "0"
            ).strip().lower()
            defer_attach_enabled = env_value not in {"0", "false", "no", "off"}

        defer_attach = defer_attach_enabled and _QTimer is not None
        if defer_attach:
            if callable(_defer_native_attach_delay_ms_fn):
                delay_ms = int(_defer_native_attach_delay_ms_fn())
            else:
                try:
                    delay_ms = max(
                        0,
                        int(
                            str(
                                os.environ.get(
                                    "FREECAD_COMMANDTAB_DEFER_NATIVE_ATTACH_DELAY_MS", "120"
                                )
                                or "120"
                            ).strip()
                        ),
                    )
                except Exception:
                    delay_ms = 120
            _StartupTrace.mark("InitGui.defer_native_attach", delayMs=int(delay_ms))
            # Try immediately first. Some host startup sequences can delay or
            # skip the first deferred timer callback, leaving the commandtab
            # inactive for the whole session.
            _finish_startup_attach()
            if startup_attach_state.get("completed") is not True:
                _QTimer.singleShot(int(delay_ms), _finish_startup_attach)
                # Guard against rare startup races where the first deferred callback
                # runs before key UI objects are fully ready.
                _QTimer.singleShot(int(delay_ms + 500), _finish_startup_attach)
        else:
            _finish_startup_attach()
    except Exception:
        startup_error = traceback.format_exc()
        try:
            _App.Console.PrintError(
                "FreeCAD CommandTab startup failed (guarded):\n"
                + startup_error
                + "\n"
            )
        except Exception:
            print(startup_error)


_safe_commandtab_startup()
