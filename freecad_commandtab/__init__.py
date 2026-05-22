"""Structured package for the FreeCAD CommandTab workbench."""
from __future__ import annotations

try:
    from . import gui_compat as _gui_compat

    _gui_compat.ensure_get_main_window()
except Exception:
    pass

