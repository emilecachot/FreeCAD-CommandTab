from __future__ import annotations

import FreeCADGui as Gui

try:
    from PySide.QtWidgets import QApplication, QMainWindow
except Exception:  # pragma: no cover - FreeCAD runtime dependency
    QApplication = None
    QMainWindow = object


def _discover_main_window():
    if QApplication is None:
        return None
    app = QApplication.instance()
    if app is None:
        return None
    try:
        for widget in app.topLevelWidgets():
            if isinstance(widget, QMainWindow):
                return widget
            try:
                meta_object = widget.metaObject()
                class_name = (
                    str(meta_object.className()).strip() if meta_object is not None else ""
                )
                if "MainWindow" in class_name:
                    return widget
            except Exception:
                continue
    except Exception:
        return None
    return None


def ensure_get_main_window() -> None:
    if callable(getattr(Gui, "getMainWindow", None)):
        return

    def _compat_get_main_window():
        return _discover_main_window()

    Gui.getMainWindow = _compat_get_main_window

