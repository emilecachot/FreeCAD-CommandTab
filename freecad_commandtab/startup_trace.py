from __future__ import annotations

import json
import os
import tempfile
import time
from contextlib import contextmanager
from pathlib import Path

_TRACE_FLAG = os.environ.get("FREECAD_COMMANDTAB_STARTUP_TRACE", "").strip().lower()
_ENABLED = _TRACE_FLAG in ["1", "true", "yes", "on"]
_START_TIME = time.perf_counter()
_SESSION_NAME = "startup"


def enabled() -> bool:
    return _ENABLED


def path() -> str:
    explicit_path = os.environ.get("FREECAD_COMMANDTAB_STARTUP_TRACE_FILE", "").strip()
    if explicit_path != "":
        return explicit_path
    return str(
        Path(tempfile.gettempdir())
        / f"freecad-commandtab-startup-trace-{os.getpid()}.jsonl"
    )


def _ensure_parent_dir(trace_path: str) -> None:
    Path(trace_path).parent.mkdir(parents=True, exist_ok=True)


def _write(payload: dict) -> None:
    if _ENABLED is False:
        return
    trace_path = path()
    _ensure_parent_dir(trace_path)
    with open(trace_path, "a", encoding="utf-8") as handle:
        handle.write(json.dumps(payload, ensure_ascii=True, separators=(",", ":")))
        handle.write("\n")


def reset_session(session_name: str = "startup") -> str:
    global _START_TIME
    global _SESSION_NAME

    _SESSION_NAME = str(session_name or "startup")
    _START_TIME = time.perf_counter()
    if _ENABLED is False:
        return path()

    trace_path = path()
    _ensure_parent_dir(trace_path)
    with open(trace_path, "w", encoding="utf-8") as handle:
        handle.write("")
    mark("session.reset", session=_SESSION_NAME)
    return trace_path


def mark(label: str, **fields) -> None:
    if _ENABLED is False:
        return
    payload = {
        "t_ms": round((time.perf_counter() - _START_TIME) * 1000.0, 3),
        "event": str(label),
        "session": _SESSION_NAME,
    }
    if fields:
        payload["fields"] = {str(key): fields[key] for key in sorted(fields)}
    _write(payload)


@contextmanager
def span(label: str, **fields):
    if _ENABLED is False:
        yield
        return

    span_start = time.perf_counter()
    mark(f"{label}.begin", **fields)
    try:
        yield
    except Exception as exc:
        payload = dict(fields)
        payload["error"] = exc.__class__.__name__
        payload["message"] = str(exc)
        payload["duration_ms"] = round((time.perf_counter() - span_start) * 1000.0, 3)
        mark(f"{label}.error", **payload)
        raise

    payload = dict(fields)
    payload["duration_ms"] = round((time.perf_counter() - span_start) * 1000.0, 3)
    mark(f"{label}.end", **payload)


__all__ = ["enabled", "mark", "path", "reset_session", "span"]
