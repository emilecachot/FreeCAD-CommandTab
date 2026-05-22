from __future__ import annotations

import json
import re
from typing import Any

import FreeCAD as App
import FreeCADGui as Gui
from PySide.QtCore import QEvent, QLocale, QObject, QSize, Qt, QTimer
from PySide.QtGui import QColor, QGuiApplication, QIcon, QKeySequence
from PySide.QtWidgets import (
    QAbstractItemView,
    QCheckBox,
    QDialog,
    QDockWidget,
    QDoubleSpinBox,
    QFormLayout,
    QFrame,
    QGridLayout,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QListWidget,
    QListWidgetItem,
    QPushButton,
    QShortcut,
    QSizePolicy,
    QVBoxLayout,
    QWidget,
)

from freecad_commandtab import paths
from freecad_commandtab import theme as CommandTabTheme

translate = App.Qt.translate

_PREFERENCES = App.ParamGet("User parameter:BaseApp/Preferences/Mod/FreeCAD-CommandTab")
_GENERAL_PREFERENCES = App.ParamGet("User parameter:BaseApp/Preferences/General")
_VIEW_PREFERENCES = App.ParamGet("User parameter:BaseApp/Preferences/View")
_PRODUCTIVITY_PRESET_VERSION = 1
_QUICK_INSPECTOR_VERSION = 1
_SELECTION_ACTION_BAR_VERSION = 1
_RECENT_LIMIT = 14
_ICON_PRODUCTIVITY_PRESET = paths.addon_path("Resources", "icons", "align-to-selection.svg")
_ICON_QUICK_INSPECTOR = paths.addon_path("Resources", "icons", "view-measurement.svg")
_ICON_SELECTION_ACTION_BAR = paths.addon_path("Resources", "icons", "view-select.svg")
_COMPACT_ENTRY_TITLES = {
    "CommandTab_CommandPalette": translate("FreeCAD CommandTab", "Palette"),
    "toggle_quick_inspector": translate("FreeCAD CommandTab", "Inspector"),
    "toggle_selection_action_bar": translate("FreeCAD CommandTab", "Action Bar"),
    "apply_productivity_preset": translate("FreeCAD CommandTab", "Preset"),
}
_DEFAULT_COMMANDS = [
    "Std_Save",
    "Std_Undo",
    "Std_Redo",
    "Std_Edit",
    "Std_TransformManip",
    "Std_ToggleVisibility",
    "Std_Delete",
]
_CONTROLLER: ProductivityController | None = None


def _resolve_theme_color(
    tokens: dict[str, Any], *token_names: str, fallback: str
) -> QColor:
    for token_name in token_names:
        color = QColor(str(tokens.get(token_name, "")))
        if color.isValid():
            return color
    return QColor(fallback)


def _rgba(color: QColor, alpha: int) -> str:
    if color.isValid() is False:
        color = QColor("#000000")
    return f"rgba({color.red()}, {color.green()}, {color.blue()}, {alpha})"


def _ergonomics_palette() -> dict[str, str | bool]:
    tokens = CommandTabTheme.current_theme_tokens()
    is_dark = bool(tokens.get("isDark", False))
    primary_text = QColor(CommandTabTheme.primary_text_color())

    shell_background = _resolve_theme_color(
        tokens,
        "shellBackground",
        "quickBackground",
        fallback="#171c24" if is_dark else "#eef2f7",
    )
    shell_border = _resolve_theme_color(
        tokens,
        "shellBorder",
        "panelCardBorder",
        fallback="#465264" if is_dark else "#cbd5e1",
    )
    surface = _resolve_theme_color(
        tokens,
        "panelCardBackground",
        "tabSelectedBackground",
        fallback="#1d2430" if is_dark else "#ffffff",
    )
    surface_alt = _resolve_theme_color(
        tokens,
        "panelFooterBackground",
        "tabBackground",
        fallback="#232b36" if is_dark else "#f8fafc",
    )
    input_background = _resolve_theme_color(
        tokens,
        "panelBodyBottom",
        "panelFooterBackground",
        fallback="#232b36" if is_dark else "#ffffff",
    )
    list_background = _resolve_theme_color(
        tokens,
        "panelBodyTop",
        "panelBodyBottom",
        fallback="#1d2430" if is_dark else "#f8fafc",
    )
    hover_background = _resolve_theme_color(
        tokens,
        "quickHoverBackground",
        "tabHoverBackground",
        fallback="#2b3950" if is_dark else "#e7f0fb",
    )
    accent = _resolve_theme_color(
        tokens,
        "tabAccent",
        "panelBodyAccent",
        fallback="#6ea8ff" if is_dark else "#2563eb",
    )
    title_text = _resolve_theme_color(
        tokens,
        "titleText",
        "buttonText",
        fallback="#f2f5fb" if is_dark else "#0f172a",
    )
    text = _resolve_theme_color(
        tokens,
        "buttonText",
        "titleText",
        fallback="#edf2fa" if is_dark else "#111827",
    )
    if primary_text.isValid():
        title_text = primary_text
        text = primary_text
    selected_background = _resolve_theme_color(
        tokens,
        "tabSelectedBackground",
        "quickHoverBackground",
        fallback="#2b3950" if is_dark else "#dbeafe",
    )

    return {
        "isDark": is_dark,
        "shellBackground": shell_background.name(),
        "shellBorder": shell_border.name(),
        "surface": surface.name(),
        "surfaceAlt": surface_alt.name(),
        "inputBackground": input_background.name(),
        "listBackground": list_background.name(),
        "hoverBackground": hover_background.name(),
        "accent": accent.name(),
        "accentSoft": _rgba(accent, 62 if is_dark else 28),
        "accentStrong": _rgba(accent, 94 if is_dark else 46),
        "floatingBackground": _rgba(shell_background, 236 if is_dark else 232),
        "titleText": title_text.name(),
        "text": text.name(),
        "mutedText": text.name(),
        "selectionBackground": selected_background.name(),
        "selectionBorder": accent.name(),
    }


def _palette_dialog_stylesheet() -> str:
    palette = _ergonomics_palette()
    return f"""
QDialog#FreeCADCommandTabProductivityPalette {{
    background: {palette["shellBackground"]};
    border: 1px solid {palette["shellBorder"]};
    border-radius: 12px;
}}
QFrame#PaletteFrame {{
    background: {palette["shellBackground"]};
    border: none;
}}
QLabel#PaletteTitle {{
    color: {palette["titleText"]};
    font-size: 16px;
    font-weight: 700;
}}
QLabel#PaletteHint {{
    color: {palette["mutedText"]};
    font-size: 11px;
}}
QLineEdit#PaletteSearch {{
    background: {palette["inputBackground"]};
    color: {palette["text"]};
    border: 1px solid {palette["shellBorder"]};
    border-radius: 10px;
    padding: 10px 12px;
    font-size: 14px;
}}
QLineEdit#PaletteSearch:focus {{
    border-color: {palette["accent"]};
}}
QListWidget#PaletteResults {{
    background: {palette["listBackground"]};
    color: {palette["text"]};
    border: 1px solid {palette["shellBorder"]};
    border-radius: 10px;
    outline: none;
    padding: 4px;
}}
QListWidget#PaletteResults::item {{
    border: 1px solid transparent;
    border-radius: 8px;
    padding: 8px 10px;
    margin: 2px 0;
}}
QListWidget#PaletteResults::item:hover {{
    background: {palette["hoverBackground"]};
    border-color: {palette["accentSoft"]};
}}
QListWidget#PaletteResults::item:selected {{
    background: {palette["selectionBackground"]};
    color: {palette["titleText"]};
    border-color: {palette["selectionBorder"]};
}}
"""


def _quick_inspector_stylesheet() -> str:
    palette = _ergonomics_palette()
    return f"""
QDockWidget#FreeCADCommandTabQuickInspector {{
    color: {palette["text"]};
}}
QWidget#QuickInspectorBody {{
    background: {palette["shellBackground"]};
}}
QFrame#QuickInspectorCard {{
    background: {palette["surface"]};
    border: 1px solid {palette["shellBorder"]};
    border-radius: 12px;
}}
QLabel#QuickInspectorTitle {{
    color: {palette["titleText"]};
    font-size: 16px;
    font-weight: 700;
}}
QLabel#QuickInspectorSubtitle {{
    color: {palette["mutedText"]};
    font-size: 11px;
}}
QLabel#QuickInspectorFieldLabel {{
    color: {palette["text"]};
    font-size: 11px;
    font-weight: 600;
}}
QLineEdit#QuickInspectorLineEdit,
QDoubleSpinBox#QuickInspectorSpin {{
    background: {palette["inputBackground"]};
    color: {palette["text"]};
    border: 1px solid {palette["shellBorder"]};
    border-radius: 8px;
    padding: 6px 8px;
    min-height: 28px;
}}
QLineEdit#QuickInspectorLineEdit:focus,
QDoubleSpinBox#QuickInspectorSpin:focus {{
    border-color: {palette["accent"]};
}}
QCheckBox#QuickInspectorCheckBox {{
    color: {palette["text"]};
    spacing: 8px;
}}
QPushButton#QuickInspectorAction {{
    background: {palette["surfaceAlt"]};
    color: {palette["text"]};
    border: 1px solid {palette["shellBorder"]};
    border-radius: 9px;
    padding: 7px 9px;
    text-align: left;
    min-height: 32px;
}}
QPushButton#QuickInspectorAction:hover {{
    background: {palette["hoverBackground"]};
    border-color: {palette["accent"]};
}}
QPushButton#QuickInspectorAction:pressed {{
    background: {palette["accentSoft"]};
    border-color: {palette["accent"]};
}}
"""


def _selection_action_bar_stylesheet() -> str:
    palette = _ergonomics_palette()
    return f"""
QFrame#FreeCADCommandTabSelectionActionBar {{
    background: {palette["floatingBackground"]};
    border: 1px solid {palette["shellBorder"]};
    border-radius: 12px;
}}
QLabel#SelectionActionBarTitle {{
    color: {palette["titleText"]};
    font-size: 12px;
    font-weight: 700;
}}
QLabel#SelectionActionBarSubtitle {{
    color: {palette["mutedText"]};
    font-size: 10px;
}}
QPushButton#SelectionActionBarButton {{
    background: {palette["surfaceAlt"]};
    color: {palette["text"]};
    border: 1px solid {palette["shellBorder"]};
    border-radius: 9px;
    padding: 5px 9px;
    min-height: 28px;
}}
QPushButton#SelectionActionBarButton:hover {{
    background: {palette["hoverBackground"]};
    border-color: {palette["accent"]};
}}
QPushButton#SelectionActionBarButton:pressed {{
    background: {palette["accentStrong"]};
    border-color: {palette["accent"]};
}}
"""


def _command(name: str):
    try:
        return Gui.Command.get(name)
    except Exception:
        return None


def _resolve_command_icon(command_id: str, icon_source: str) -> QIcon:
    # Prefer the live QAction icon — FreeCAD has already resolved it
    cmd = _command(command_id)
    if cmd is not None:
        try:
            action = cmd.getAction()
            if action is not None:
                icon = action.icon() if not isinstance(action, list) else (action[0].icon() if action else QIcon())
                if not icon.isNull():
                    return icon
        except Exception:
            pass

    if not icon_source:
        return QIcon()

    # Try the source directly (handles ":/" resources and absolute paths)
    icon = QIcon(icon_source)
    if not icon.isNull():
        return icon

    # Try with "icons:" search path prefix (FreeCAD registers addon icon dirs here)
    if not icon_source.startswith("icons:") and not icon_source.startswith(":/"):
        icon = QIcon("icons:" + icon_source)
        if not icon.isNull():
            return icon

    return QIcon()


def _current_workbench_name() -> str:
    try:
        active = Gui.activeWorkbench()
    except Exception:
        return ""

    for attribute in ["name", "__class__"]:
        try:
            value = getattr(active, attribute)
            if attribute == "__class__":
                value = value.__name__
            elif callable(value):
                value = value()
            value = str(value).strip()
            if value != "":
                return value
        except Exception:
            continue
    return ""


def _workbench_title(workbench_name: str) -> str:
    try:
        workbench = Gui.getWorkbench(workbench_name)
    except Exception:
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
            continue
    else:
        value = workbench_name

    cleaned = str(value).replace("&", "").strip()
    if cleaned in ["", workbench_name] or cleaned.endswith("Workbench"):
        cleaned = re.sub(r"Workbench$", "", cleaned).strip()
        cleaned = re.sub(r"(?<!^)([A-Z])", r" \1", cleaned).strip()
        cleaned = re.sub(r"\s+", " ", cleaned).strip()
    return cleaned or workbench_name


def _current_locale_signature() -> str:
    try:
        locale_name = str(Gui.getLocale() or "").strip()
        if locale_name not in ["", "C"]:
            return locale_name
    except Exception:
        pass

    try:
        configured_language = str(_GENERAL_PREFERENCES.GetString("Language") or "").strip()
        if configured_language not in ["", "C"]:
            return configured_language
    except Exception:
        pass

    try:
        system_locale = str(QLocale.system().name() or "").strip()
        if system_locale != "":
            return system_locale
    except Exception:
        pass

    return "C"


def _localized_command_info(command_name: str) -> dict[str, Any]:
    try:
        import Standard_Functions_CommandTab as StandardFunctions

        info = StandardFunctions.CommandInfoCorrections(command_name)
        if isinstance(info, dict):
            return dict(info)
    except Exception:
        pass
    return {}


def _command_display_title(command_name: str) -> str:
    info = _localized_command_info(command_name)
    return str(
        info.get("DisplayText")
        or info.get("ActionText")
        or info.get("menuText")
        or ""
    ).replace("&", "").strip()


def _selected_objects() -> list[Any]:
    try:
        return list(Gui.Selection.getSelection())
    except Exception:
        return []


def _property_type_id(obj: Any, property_name: str) -> str:
    try:
        return str(obj.getTypeIdOfProperty(property_name) or "")
    except Exception:
        return ""


def _property_numeric_value(obj: Any, property_name: str) -> float | None:
    try:
        value = getattr(obj, property_name)
    except Exception:
        return None

    try:
        if hasattr(value, "Value"):
            return float(value.Value)
        return float(value)
    except Exception:
        return None


def _is_numeric_property(obj: Any, property_name: str) -> bool:
    type_id = _property_type_id(obj, property_name)
    if any(
        token in type_id
        for token in [
            "Length",
            "Distance",
            "Float",
            "Angle",
            "Integer",
            "Quantity",
        ]
    ):
        return True
    return _property_numeric_value(obj, property_name) is not None


def _compact_entry_title(entry: dict[str, Any]) -> str:
    identifier = str(entry.get("id", ""))
    native_title = _command_display_title(identifier)
    if native_title != "":
        title = native_title
    elif identifier in _COMPACT_ENTRY_TITLES:
        title = _COMPACT_ENTRY_TITLES[identifier]
    else:
        title = str(entry.get("title", "")).strip()
    if len(title) <= 18:
        return title
    if " / " in title:
        return title.split(" / ", 1)[0].strip()
    if "..." in title:
        return title.replace("...", "").strip()
    return title[:18].rstrip() + "…"


def _load_string_list(setting_name: str) -> list[str]:
    raw_value = _PREFERENCES.GetString(setting_name)
    if raw_value == "":
        return []

    try:
        value = json.loads(raw_value)
    except Exception:
        return []

    if isinstance(value, list) is False:
        return []
    return [str(item) for item in value if str(item).strip() != ""]


def _save_string_list(setting_name: str, values: list[str]) -> None:
    _PREFERENCES.SetString(
        setting_name,
        json.dumps(values[:_RECENT_LIMIT], ensure_ascii=True, separators=(",", ":")),
    )
    App.saveParameter()


def _title_acronym(title: str) -> str:
    words = [w for w in title.split() if w]
    return "".join(w[0] for w in words)


def _fuzzy_sequential(text: str, query: str) -> bool:
    idx = 0
    for ch in query:
        found = text.find(ch, idx)
        if found == -1:
            return False
        idx = found + 1
    return True


def _score_text(
    query: str,
    entry: dict[str, Any],
    recent_commands: list[str],
    recent_workbenches: list[str],
    current_wb_commands: set[str] | None = None,
):
    normalized_query = query.strip().lower()
    if normalized_query == "":
        return None

    search_text = str(entry.get("search", ""))
    if search_text == "":
        return None

    title = str(entry.get("titleLower", ""))
    identifier = str(entry.get("idLower", ""))

    tokens = [token for token in normalized_query.split(" ") if token]

    # Primary match: all tokens present in search corpus
    all_tokens_match = not any(token not in search_text for token in tokens)

    # Acronym match: query matches initials of title words (e.g. "pb" → "Part Boolean")
    acronym_title = _title_acronym(title)
    id_parts = re.split(r"[_\s]+", identifier)
    acronym_id = "".join(p[0] for p in id_parts if p)
    exact_acronym = normalized_query in (acronym_title, acronym_id)
    prefix_acronym = (
        not exact_acronym
        and len(normalized_query) >= 2
        and (acronym_title.startswith(normalized_query) or acronym_id.startswith(normalized_query))
    )

    # Fuzzy sequential: each query char appears in order in title or identifier
    fuzzy_hit = (
        not all_tokens_match
        and not exact_acronym
        and not prefix_acronym
        and len(normalized_query) >= 2
        and (_fuzzy_sequential(title, normalized_query) or _fuzzy_sequential(identifier, normalized_query))
    )

    if not (all_tokens_match or exact_acronym or prefix_acronym or fuzzy_hit):
        return None

    score = 1000

    # Tier the match quality
    if exact_acronym:
        score -= 480
    elif prefix_acronym:
        score -= 400
    elif fuzzy_hit:
        score -= 80  # fuzzy is a weak signal — don't over-promote

    # Exact / prefix / contains bonuses on title and id
    if title.startswith(normalized_query):
        score -= 550
    elif normalized_query in title:
        score -= 360 - max(0, title.index(normalized_query))

    if identifier.startswith(normalized_query):
        score -= 330
    elif normalized_query in identifier:
        score -= 220 - max(0, identifier.index(normalized_query))

    # Boost commands from the active workbench
    if current_wb_commands and entry["kind"] == "command" and entry["id"] in current_wb_commands:
        score -= 150

    if entry["kind"] == "action":
        score -= 80

    if entry["kind"] == "command" and entry["id"] in recent_commands:
        score -= 120 - recent_commands.index(entry["id"])

    if entry["kind"] == "workbench" and entry["id"] in recent_workbenches:
        score -= 120 - recent_workbenches.index(entry["id"])

    return score


class ProductivityPaletteDialog(QDialog):
    def __init__(self, controller: ProductivityController, parent=None):
        super().__init__(parent)
        self._controller = controller
        self.setObjectName("FreeCADCommandTabProductivityPalette")
        self.setWindowTitle(translate("FreeCAD CommandTab", "Command Palette"))
        self.setModal(True)
        self.resize(760, 480)
        self.setWindowIcon(QIcon(_ICON_SELECTION_ACTION_BAR))
        self.apply_theme()

        frame = QFrame(self)
        frame.setObjectName("PaletteFrame")
        outer_layout = QVBoxLayout(self)
        outer_layout.setContentsMargins(0, 0, 0, 0)
        outer_layout.addWidget(frame)

        layout = QVBoxLayout(frame)
        layout.setContentsMargins(18, 16, 18, 16)
        layout.setSpacing(10)

        title_row = QHBoxLayout()
        title_row.setContentsMargins(0, 0, 0, 0)
        title_row.setSpacing(8)

        title = QLabel(translate("FreeCAD CommandTab", "Quick Command Palette"), frame)
        title.setObjectName("PaletteTitle")
        title_row.addWidget(title, 1)

        hint = QLabel(
            translate("FreeCAD CommandTab", "Commands, workbenches, ergonomic actions"),
            frame,
        )
        hint.setObjectName("PaletteHint")
        hint.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        title_row.addWidget(hint)
        layout.addLayout(title_row)

        self.search_edit = QLineEdit(frame)
        self.search_edit.setObjectName("PaletteSearch")
        self.search_edit.setPlaceholderText(
            translate("FreeCAD CommandTab", "Type a command, workbench or shortcut")
        )
        layout.addWidget(self.search_edit)

        self.results = QListWidget(frame)
        self.results.setObjectName("PaletteResults")
        self.results.setSelectionMode(QAbstractItemView.SingleSelection)
        self.results.setAlternatingRowColors(False)
        self.results.setUniformItemSizes(False)
        self.results.setIconSize(QSize(18, 18))
        layout.addWidget(self.results, 1)

        footer = QLabel(
            translate("FreeCAD CommandTab", "Enter runs. Esc closes. Ctrl+Space reopens."),
            frame,
        )
        footer.setObjectName("PaletteHint")
        layout.addWidget(footer)

        self.search_edit.textChanged.connect(self._refresh_results)
        self.search_edit.returnPressed.connect(self._activate_selected_item)
        self.results.itemActivated.connect(self._activate_item)

        # Arrow down from search box moves focus to the list; typing in list refocuses search
        self.search_edit.installEventFilter(self)
        self.results.installEventFilter(self)

    def apply_theme(self) -> None:
        self.setStyleSheet(_palette_dialog_stylesheet())

    def show_palette(self) -> None:
        self.apply_theme()
        parent = self.parentWidget()
        screen_geometry = None
        screen = self.screen()
        if screen is None and parent is not None:
            try:
                window_handle = parent.windowHandle()
            except Exception:
                window_handle = None
            if window_handle is not None:
                try:
                    screen = window_handle.screen()
                except Exception:
                    screen = None
        if screen is None:
            try:
                screen = QGuiApplication.primaryScreen()
            except Exception:
                screen = None
        if screen is not None:
            try:
                screen_geometry = screen.availableGeometry()
            except Exception:
                screen_geometry = None

        if parent is not None:
            geometry = parent.geometry()
            width = min(860, max(560, geometry.width() - 120))
            height = min(580, max(380, geometry.height() - 140))
            if screen_geometry is not None:
                width = min(width, max(460, screen_geometry.width() - 40))
                height = min(height, max(320, screen_geometry.height() - 56))
            left = geometry.x() + max(20, int((geometry.width() - width) / 2))
            top = geometry.y() + max(36, int(geometry.height() * 0.14))
            if screen_geometry is not None:
                left = max(screen_geometry.left() + 20, left)
                top = max(screen_geometry.top() + 20, top)
                left = min(left, screen_geometry.right() - width - 20)
                top = min(top, screen_geometry.bottom() - height - 20)
            self.setGeometry(left, top, width, height)

        self.search_edit.clear()
        self._refresh_results("")
        self.show()
        try:
            self.raise_()
        except Exception:
            pass
        self.activateWindow()
        self.search_edit.setFocus(Qt.OtherFocusReason)

    def _refresh_results(self, query: str) -> None:
        self.results.clear()
        entries = self._controller.query_entries(query)

        if len(entries) == 0:
            item = QListWidgetItem(
                translate("FreeCAD CommandTab", "No matching command or workbench"),
                self.results,
            )
            item.setFlags(item.flags() & ~Qt.ItemIsEnabled)
            return

        for entry in entries:
            item = QListWidgetItem(self.results)
            title = entry["title"]
            subtitle = entry["subtitle"]
            shortcut = entry["shortcut"]
            workbench = entry.get("workbench", "")

            # Build secondary line: workbench context · tooltip · shortcut
            parts = []
            if workbench:
                parts.append(workbench)
            if subtitle and subtitle != title:
                parts.append(subtitle)
            if shortcut:
                parts.append(shortcut)
            secondary = "  ·  ".join(parts)

            item.setText(f"{title}\n{secondary}".strip())
            item.setData(Qt.UserRole, entry)
            row_height = max(52, self.results.fontMetrics().lineSpacing() * 2 + 18)
            item.setSizeHint(QSize(0, row_height))
            icon = _resolve_command_icon(entry.get("id", ""), entry.get("icon", ""))
            if not icon.isNull():
                item.setIcon(icon)

        self.results.setCurrentRow(0)

    def _activate_selected_item(self) -> None:
        item = self.results.currentItem()
        if item is None:
            return
        self._activate_item(item)

    def _activate_item(self, item: QListWidgetItem) -> None:
        payload = item.data(Qt.UserRole)
        if isinstance(payload, dict) is False:
            return

        self.hide()
        self._controller.activate_entry(payload)

    def eventFilter(self, watched, event) -> bool:
        if event is None:
            return False
        if watched is self.search_edit and event.type() == QEvent.KeyPress:
            key = event.key()
            if key == Qt.Key_Down:
                self.results.setFocus(Qt.OtherFocusReason)
                if self.results.currentRow() < 0 and self.results.count() > 0:
                    self.results.setCurrentRow(0)
                return True
            if key == Qt.Key_Up:
                if self.results.currentRow() <= 0:
                    return True
                self.results.setFocus(Qt.OtherFocusReason)
                return True
        if watched is self.results and event.type() == QEvent.KeyPress:
            key = event.key()
            # Printable char or Backspace while list has focus: redirect to search box
            if key == Qt.Key_Backspace or (
                key not in (Qt.Key_Up, Qt.Key_Down, Qt.Key_Return, Qt.Key_Enter, Qt.Key_Escape)
                and event.text()
                and event.text().isprintable()
            ):
                self.search_edit.setFocus(Qt.OtherFocusReason)
                self.search_edit.event(event)
                return True
        return False


class SelectionQuickInspectorDock(QDockWidget):
    def __init__(self, controller: ProductivityController, parent=None):
        super().__init__(translate("FreeCAD CommandTab", "Quick Inspector"), parent)
        self._controller = controller
        self._current_objects: list[Any] = []
        self._current_action_entries: list[dict[str, Any]] = []
        self.setObjectName("FreeCADCommandTabQuickInspector")
        self.setAllowedAreas(Qt.LeftDockWidgetArea | Qt.RightDockWidgetArea)
        self.setFeatures(QDockWidget.DockWidgetMovable | QDockWidget.DockWidgetClosable)
        self.setMinimumWidth(320)
        self.setWindowIcon(QIcon(_ICON_QUICK_INSPECTOR))
        self.apply_theme()

        root = QWidget(self)
        root.setObjectName("QuickInspectorBody")
        self.setWidget(root)

        outer_layout = QVBoxLayout(root)
        outer_layout.setContentsMargins(10, 10, 10, 10)
        outer_layout.setSpacing(10)

        header_card = QFrame(root)
        header_card.setObjectName("QuickInspectorCard")
        header_layout = QVBoxLayout(header_card)
        header_layout.setContentsMargins(14, 14, 14, 14)
        header_layout.setSpacing(4)

        self._title_label = QLabel(translate("FreeCAD CommandTab", "No selection"), header_card)
        self._title_label.setObjectName("QuickInspectorTitle")
        header_layout.addWidget(self._title_label)

        self._subtitle_label = QLabel(
            translate("FreeCAD CommandTab", "Select an object to inspect and edit it quickly"),
            header_card,
        )
        self._subtitle_label.setObjectName("QuickInspectorSubtitle")
        self._subtitle_label.setWordWrap(True)
        header_layout.addWidget(self._subtitle_label)
        outer_layout.addWidget(header_card)

        self._actions_card = QFrame(root)
        self._actions_card.setObjectName("QuickInspectorCard")
        actions_layout = QVBoxLayout(self._actions_card)
        actions_layout.setContentsMargins(12, 12, 12, 12)
        actions_layout.setSpacing(8)

        actions_title = QLabel(translate("FreeCAD CommandTab", "Suggested Actions"), self._actions_card)
        actions_title.setObjectName("QuickInspectorFieldLabel")
        actions_layout.addWidget(actions_title)

        self._actions_grid = QGridLayout()
        self._actions_grid.setContentsMargins(0, 0, 0, 0)
        self._actions_grid.setHorizontalSpacing(8)
        self._actions_grid.setVerticalSpacing(8)
        actions_layout.addLayout(self._actions_grid)
        outer_layout.addWidget(self._actions_card)

        self._fields_card = QFrame(root)
        self._fields_card.setObjectName("QuickInspectorCard")
        fields_layout = QVBoxLayout(self._fields_card)
        fields_layout.setContentsMargins(12, 12, 12, 12)
        fields_layout.setSpacing(8)

        fields_title = QLabel(translate("FreeCAD CommandTab", "Quick Properties"), self._fields_card)
        fields_title.setObjectName("QuickInspectorFieldLabel")
        fields_layout.addWidget(fields_title)

        self._fields_form = QFormLayout()
        self._fields_form.setContentsMargins(0, 0, 0, 0)
        self._fields_form.setHorizontalSpacing(10)
        self._fields_form.setVerticalSpacing(8)
        fields_layout.addLayout(self._fields_form)
        outer_layout.addWidget(self._fields_card)
        outer_layout.addStretch(1)

    def apply_theme(self) -> None:
        self.setStyleSheet(_quick_inspector_stylesheet())

    def refresh_selection(self, objects: list[Any], action_entries: list[dict[str, Any]]) -> None:
        self._current_objects = list(objects)
        self._current_action_entries = list(action_entries)
        self._populate_actions(action_entries)
        self._clear_form()

        if len(objects) == 0:
            self._title_label.setText(translate("FreeCAD CommandTab", "No selection"))
            self._subtitle_label.setText(
                translate(
                    "FreeCAD CommandTab",
                    "Select an object to expose quick actions, visibility and editable properties",
                )
            )
            self._add_message_row(
                translate("FreeCAD CommandTab", "No object selected. Use the palette or pick an object.")
            )
            return

        if len(objects) > 1:
            labels = [str(getattr(obj, "Label", getattr(obj, "Name", ""))) for obj in objects[:3]]
            extra = ""
            if len(objects) > 3:
                extra = translate("FreeCAD CommandTab", " and {} more").format(len(objects) - 3)
            self._title_label.setText(
                translate("FreeCAD CommandTab", "{} objects selected").format(len(objects))
            )
            self._subtitle_label.setText(", ".join(label for label in labels if label) + extra)
            self._add_message_row(
                translate(
                    "FreeCAD CommandTab",
                    "Quick editing is available for a single object selection.",
                )
            )
            return

        obj = objects[0]
        label = str(getattr(obj, "Label", getattr(obj, "Name", ""))).strip() or str(
            getattr(obj, "Name", "")
        )
        type_id = str(getattr(obj, "TypeId", "")).strip()
        document_name = str(getattr(getattr(obj, "Document", None), "Name", "")).strip()
        subtitle_parts = [part for part in [type_id, document_name] if part]
        self._title_label.setText(label)
        self._subtitle_label.setText("  |  ".join(subtitle_parts))

        self._add_label_editor(obj)
        self._add_visibility_editor(obj)
        self._add_placement_editors(obj)
        self._add_numeric_property_editors(obj)

    def _clear_layout(self, layout) -> None:
        while layout.count():
            item = layout.takeAt(0)
            widget = item.widget()
            child_layout = item.layout()
            if widget is not None:
                widget.deleteLater()
            if child_layout is not None:
                self._clear_layout(child_layout)

    def _clear_form(self) -> None:
        while self._fields_form.rowCount():
            self._fields_form.removeRow(0)

    def _populate_actions(self, action_entries: list[dict[str, Any]]) -> None:
        self._clear_layout(self._actions_grid)
        self._actions_card.setVisible(len(action_entries) > 0)
        if len(action_entries) == 0:
            return

        available_width = max(self.width(), self.minimumWidth(), self._actions_card.width())
        column_count = 1 if available_width < 430 else 2
        for column_index in range(2):
            self._actions_grid.setColumnStretch(column_index, 1 if column_index < column_count else 0)

        for index, entry in enumerate(action_entries[:8]):
            button = QPushButton(_compact_entry_title(entry), self._actions_card)
            button.setObjectName("QuickInspectorAction")
            button.setToolTip(entry.get("subtitle", ""))
            button.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
            button.setMinimumWidth(0)
            if entry.get("icon"):
                button.setIcon(QIcon(entry["icon"]))
            button.clicked.connect(lambda _checked=False, payload=dict(entry): self._controller.activate_entry(payload))
            self._actions_grid.addWidget(button, int(index / column_count), index % column_count)

    def resizeEvent(self, event) -> None:
        super().resizeEvent(event)
        if len(self._current_action_entries) > 0:
            self._populate_actions(self._current_action_entries)

    def _add_message_row(self, message: str) -> None:
        message_label = QLabel(message, self._fields_card)
        message_label.setObjectName("QuickInspectorSubtitle")
        message_label.setWordWrap(True)
        self._fields_form.addRow(message_label)

    def _add_row_label(self, text: str) -> QLabel:
        label = QLabel(text, self._fields_card)
        label.setObjectName("QuickInspectorFieldLabel")
        return label

    def _add_label_editor(self, obj: Any) -> None:
        line_edit = QLineEdit(self._fields_card)
        line_edit.setObjectName("QuickInspectorLineEdit")
        line_edit.setText(str(getattr(obj, "Label", getattr(obj, "Name", ""))))
        line_edit.editingFinished.connect(
            lambda obj=obj, widget=line_edit: self._apply_label_change(obj, widget.text())
        )
        self._fields_form.addRow(self._add_row_label(translate("FreeCAD CommandTab", "Label")), line_edit)

    def _add_visibility_editor(self, obj: Any) -> None:
        view_object = getattr(obj, "ViewObject", None)
        if view_object is None or hasattr(view_object, "Visibility") is False:
            return

        checkbox = QCheckBox(translate("FreeCAD CommandTab", "Visible"), self._fields_card)
        checkbox.setObjectName("QuickInspectorCheckBox")
        checkbox.setChecked(bool(view_object.Visibility))
        checkbox.toggled.connect(
            lambda checked, view_object=view_object: self._apply_visibility_change(view_object, checked)
        )
        self._fields_form.addRow(
            self._add_row_label(translate("FreeCAD CommandTab", "Display")), checkbox
        )

    def _add_placement_editors(self, obj: Any) -> None:
        placement = getattr(obj, "Placement", None)
        base = getattr(placement, "Base", None)
        if base is None:
            return

        for axis_name in ["x", "y", "z"]:
            spin_box = self._create_spin_box(getattr(base, axis_name))
            spin_box.editingFinished.connect(
                lambda axis_name=axis_name, obj=obj, widget=spin_box: self._apply_placement_change(
                    obj, axis_name, widget.value()
                )
            )
            self._fields_form.addRow(
                self._add_row_label(f"Pos {axis_name.upper()}"),
                spin_box,
            )

    def _add_numeric_property_editors(self, obj: Any) -> None:
        properties = list(getattr(obj, "PropertiesList", []))
        if len(properties) == 0:
            return

        preferred_order = [
            "Length",
            "Width",
            "Height",
            "Radius",
            "Radius1",
            "Radius2",
            "Diameter",
            "Angle",
            "Offset",
            "Pitch",
            "Size",
        ]
        ordered_properties = [name for name in preferred_order if name in properties]
        ordered_properties.extend(
            name
            for name in properties
            if name not in ordered_properties
            and name not in ["Label", "Placement", "ExpressionEngine", "Proxy", "Shape"]
        )

        added = 0
        for property_name in ordered_properties:
            if _is_numeric_property(obj, property_name) is False:
                continue
            current_value = _property_numeric_value(obj, property_name)
            if current_value is None:
                continue

            spin_box = self._create_spin_box(current_value)
            spin_box.editingFinished.connect(
                lambda obj=obj, property_name=property_name, widget=spin_box: self._apply_numeric_property_change(
                    obj, property_name, widget.value()
                )
            )
            self._fields_form.addRow(self._add_row_label(property_name), spin_box)
            added += 1
            if added >= 6:
                break

    def _create_spin_box(self, value: float) -> QDoubleSpinBox:
        spin_box = QDoubleSpinBox(self._fields_card)
        spin_box.setObjectName("QuickInspectorSpin")
        spin_box.setDecimals(3)
        spin_box.setRange(-1000000000.0, 1000000000.0)
        spin_box.setSingleStep(1.0)
        spin_box.setKeyboardTracking(False)
        spin_box.setValue(float(value))
        return spin_box

    def _apply_label_change(self, obj: Any, value: str) -> None:
        cleaned = str(value).strip()
        if cleaned == "":
            return
        try:
            if str(getattr(obj, "Label", "")) == cleaned:
                return
            obj.Label = cleaned
            self._refresh_after_change(obj)
        except Exception:
            self._controller.show_status(
                translate("FreeCAD CommandTab", "Unable to rename the selected object"),
                5000,
            )

    def _apply_visibility_change(self, view_object: Any, checked: bool) -> None:
        try:
            if bool(getattr(view_object, "Visibility", True)) == bool(checked):
                return
            view_object.Visibility = bool(checked)
            Gui.updateGui()
        except Exception:
            self._controller.show_status(
                translate("FreeCAD CommandTab", "Unable to update object visibility"),
                5000,
            )

    def _apply_placement_change(self, obj: Any, axis_name: str, value: float) -> None:
        try:
            placement = obj.Placement
            base = App.Vector(placement.Base.x, placement.Base.y, placement.Base.z)
            if abs(float(getattr(base, axis_name)) - float(value)) < 0.0001:
                return
            setattr(base, axis_name, float(value))
            obj.Placement = App.Placement(base, placement.Rotation)
            self._refresh_after_change(obj)
        except Exception:
            self._controller.show_status(
                translate("FreeCAD CommandTab", "Unable to update object placement"),
                5000,
            )

    def _apply_numeric_property_change(self, obj: Any, property_name: str, value: float) -> None:
        try:
            current_value = _property_numeric_value(obj, property_name)
            if current_value is not None and abs(float(current_value) - float(value)) < 0.0001:
                return
            setattr(obj, property_name, float(value))
            self._refresh_after_change(obj)
        except Exception:
            self._controller.show_status(
                translate("FreeCAD CommandTab", "Unable to update {}").format(property_name),
                5000,
            )

    def _refresh_after_change(self, obj: Any) -> None:
        document = getattr(obj, "Document", None)
        if document is not None:
            try:
                document.recompute()
            except Exception:
                pass
        try:
            Gui.updateGui()
        except Exception:
            pass
        self._controller.refresh_selection_tools()


class SelectionActionBar(QFrame):
    def __init__(self, controller: ProductivityController, parent=None):
        super().__init__(parent)
        self._controller = controller
        self._entries: list[dict[str, Any]] = []
        self.setObjectName("FreeCADCommandTabSelectionActionBar")
        self.setFrameShape(QFrame.NoFrame)
        self.setAttribute(Qt.WA_StyledBackground, True)
        self.apply_theme()

        root_layout = QVBoxLayout(self)
        root_layout.setContentsMargins(12, 10, 12, 10)
        root_layout.setSpacing(8)

        self._title = QLabel(translate("FreeCAD CommandTab", "Selection Actions"), self)
        self._title.setObjectName("SelectionActionBarTitle")
        root_layout.addWidget(self._title)

        self._subtitle = QLabel("", self)
        self._subtitle.setObjectName("SelectionActionBarSubtitle")
        self._subtitle.setWordWrap(True)
        root_layout.addWidget(self._subtitle)

        self._buttons_layout = QHBoxLayout()
        self._buttons_layout.setContentsMargins(0, 0, 0, 0)
        self._buttons_layout.setSpacing(6)
        root_layout.addLayout(self._buttons_layout)

        self.hide()

    def apply_theme(self) -> None:
        self.setStyleSheet(_selection_action_bar_stylesheet())

    def set_state(self, objects: list[Any], entries: list[dict[str, Any]], enabled: bool) -> None:
        self._entries = list(entries)
        self._clear_buttons()

        if enabled is False or len(objects) == 0 or len(entries) == 0:
            self.hide()
            return

        if len(objects) == 1:
            obj = objects[0]
            label = str(getattr(obj, "Label", getattr(obj, "Name", ""))).strip()
            self._title.setText(label or translate("FreeCAD CommandTab", "Selection Actions"))
            self._subtitle.setText(str(getattr(obj, "TypeId", "")).strip())
        else:
            self._title.setText(
                translate("FreeCAD CommandTab", "{} objects selected").format(len(objects))
            )
            self._subtitle.setText(
                translate("FreeCAD CommandTab", "Fast commands for the current multi-selection")
            )

        for entry in entries[:4]:
            button = QPushButton(_compact_entry_title(entry), self)
            button.setObjectName("SelectionActionBarButton")
            button.setToolTip(entry.get("subtitle", ""))
            button.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
            if entry.get("icon"):
                button.setIcon(QIcon(entry["icon"]))
            target_width = min(168, max(96, button.sizeHint().width() + 10))
            button.setFixedWidth(target_width)
            button.clicked.connect(
                lambda _checked=False, payload=dict(entry): self._controller.activate_entry(payload)
            )
            self._buttons_layout.addWidget(button)

        self.adjustSize()
        self._reposition()
        self.show()
        self.raise_()

    def _clear_buttons(self) -> None:
        while self._buttons_layout.count():
            item = self._buttons_layout.takeAt(0)
            widget = item.widget()
            if widget is not None:
                widget.deleteLater()

    def _reposition(self) -> None:
        parent = self.parentWidget()
        if parent is None:
            return

        self.adjustSize()
        reference_widget = None
        try:
            reference_widget = parent.centralWidget()
        except Exception:
            reference_widget = None
        if reference_widget is None:
            reference_widget = parent

        reference_rect = reference_widget.geometry()
        width = self.sizeHint().width()
        height = self.sizeHint().height()
        status_bar = None
        try:
            status_bar = parent.statusBar()
        except Exception:
            status_bar = None
        bottom_margin = 20 + (status_bar.height() if status_bar is not None else 0)
        x = max(reference_rect.left() + 16, reference_rect.right() - width - 20)
        y = max(reference_rect.top() + 24, reference_rect.bottom() - height - bottom_margin)
        self.move(x, y)

    def sync_position(self) -> None:
        if self.isVisible():
            self._reposition()


class ProductivityController(QObject):
    def __init__(self):
        super().__init__()
        self._main_window = None
        self._installed = False
        self._palette = None
        self._inspector = None
        self._selection_bar = None
        self._main_window_event_filter_installed = False
        self._palette_shortcut = None
        self._preset_shortcut = None
        self._inspector_shortcut = None
        self._selection_bar_shortcut = None
        self._selection_refresh_timer = None
        self._selection_observer_installed = False
        self._command_catalog_built = False
        self._workbench_catalog_built = False
        self._action_tracking_installed = False
        self._commands: dict[str, dict[str, Any]] = {}
        self._workbenches: dict[str, dict[str, Any]] = {}
        self._actions: dict[str, dict[str, Any]] = {}
        self._current_wb_commands: set[str] = set()
        self._theme_signature = ""
        self._locale_signature = ""

    def install(self) -> bool:
        if self._installed:
            return True

        try:
            self._main_window = Gui.getMainWindow()
        except Exception:
            return False

        if self._main_window is None:
            return False

        self._selection_refresh_timer = QTimer(self)
        self._selection_refresh_timer.setSingleShot(True)
        self._selection_refresh_timer.timeout.connect(self._refresh_selection_state)

        self._install_shortcuts()
        self._rebuild_actions()
        self._refresh_current_wb_commands()
        self._install_selection_observer()

        try:
            self._main_window.workbenchActivated.connect(self._on_workbench_activated)
        except Exception:
            pass

        self._installed = True
        self._refresh_theme_chrome(force=True)
        self._apply_default_preset_if_needed()
        self._restore_inspector_visibility()
        self._restore_selection_bar_enabled()
        if (self._inspector is not None and self._inspector.isVisible()) or _PREFERENCES.GetBool(
            "SelectionActionBarEnabled"
        ):
            self._schedule_selection_refresh()
        return True

    def show_palette(self) -> None:
        if self.install() is False:
            return
        self._ensure_locale_catalogs_current()
        self._refresh_theme_chrome()
        self._ensure_palette()
        self._ensure_full_workbench_catalog()
        self._ensure_full_command_catalog()
        if self._palette is None:
            return
        self._palette.show_palette()

    def toggle_quick_inspector(self) -> None:
        if self.install() is False:
            return
        self._ensure_inspector()
        self._refresh_theme_chrome()
        if self._inspector is None:
            return
        self._inspector.setVisible(not self._inspector.isVisible())
        if self._inspector.isVisible():
            self._schedule_selection_refresh()

    def refresh_selection_tools(self) -> None:
        if self._installed is False:
            return
        self._refresh_selection_state()

    def toggle_selection_action_bar(self) -> None:
        if self.install() is False:
            return
        enabled = not _PREFERENCES.GetBool("SelectionActionBarEnabled")
        _PREFERENCES.SetBool("SelectionActionBarEnabled", enabled)
        App.saveParameter()
        if enabled:
            self._ensure_selection_bar()
        self._refresh_selection_state()

    def show_status(self, message: str, timeout_ms: int = 4000) -> None:
        if self._main_window is None or self._main_window.statusBar() is None:
            return
        self._main_window.statusBar().showMessage(message, timeout_ms)

    def activate_entry(self, entry: dict[str, Any]) -> None:
        kind = entry["kind"]
        identifier = entry["id"]

        if kind == "command":
            self._run_command(identifier)
            return

        if kind == "workbench":
            self._record_recent("ProductivityRecentWorkbenches", identifier)
            try:
                Gui.activateWorkbench(identifier)
            except Exception:
                pass
            return

        if kind == "action" and identifier == "apply_productivity_preset":
            self.apply_productivity_preset()
            return

        if kind == "action" and identifier == "toggle_quick_inspector":
            self.toggle_quick_inspector()
            return

        if kind == "action" and identifier == "toggle_selection_action_bar":
            self.toggle_selection_action_bar()

    def query_entries(self, query: str) -> list[dict[str, Any]]:
        self._ensure_locale_catalogs_current()
        normalized_query = query.strip().lower()
        recent_commands = _load_string_list("ProductivityRecentCommands")
        recent_workbenches = _load_string_list("ProductivityRecentWorkbenches")

        if normalized_query == "":
            return self._default_entries(recent_commands, recent_workbenches)

        current_wb_commands = getattr(self, "_current_wb_commands", None)

        matches: list[tuple[int, str, dict[str, Any]]] = []
        for source_name, source in [
            ("action", self._actions),
            ("command", self._commands),
            ("workbench", self._workbenches),
        ]:
            for entry in source.values():
                score = _score_text(
                    normalized_query, entry, recent_commands, recent_workbenches, current_wb_commands
                )
                if score is None:
                    continue
                matches.append((score, source_name, entry))

        matches.sort(key=lambda item: (item[0], item[2]["title"]))
        return [entry for _, _, entry in matches[:40]]

    def apply_productivity_preset(self) -> None:
        _VIEW_PREFERENCES.SetBool("SameStyleForAllViews", True)
        _VIEW_PREFERENCES.SetString("NavigationStyle", "Gui::SolidWorksNavigationStyle")
        _VIEW_PREFERENCES.SetBool("ZoomAtCursor", True)
        _VIEW_PREFERENCES.SetBool("ResetCursorPosition", False)
        _VIEW_PREFERENCES.SetInt("RotationMode", 1)
        _VIEW_PREFERENCES.SetFloat("Sensitivity", 1.35)

        _PREFERENCES.SetInt("ProductivityPresetVersion", _PRODUCTIVITY_PRESET_VERSION)
        _PREFERENCES.SetString("ProductivityPaletteShortcut", "Ctrl+Space")
        _PREFERENCES.SetString("ProductivityInspectorShortcut", "Ctrl+Shift+I")
        _PREFERENCES.SetString("SelectionActionBarShortcut", "Ctrl+Shift+A")
        App.saveParameter()

        command = _command("CommandTab_CommandPalette")
        if command is not None:
            try:
                command.setShortcut("Ctrl+Space")
            except Exception:
                pass

        inspector_command = _command("CommandTab_ToggleQuickInspector")
        if inspector_command is not None:
            try:
                inspector_command.setShortcut("Ctrl+Shift+I")
            except Exception:
                pass

        selection_bar_command = _command("CommandTab_ToggleSelectionActionBar")
        if selection_bar_command is not None:
            try:
                selection_bar_command.setShortcut("Ctrl+Shift+A")
            except Exception:
                pass

        try:
            active_document = getattr(Gui, "ActiveDocument", None)
            if active_document is not None and active_document.ActiveView is not None:
                active_document.ActiveView.setNavigationType("Gui::SolidWorksNavigationStyle")
        except Exception:
            pass

        if self._main_window is not None and self._main_window.statusBar() is not None:
            self._main_window.statusBar().showMessage(
                translate(
                    "FreeCAD CommandTab",
                    "Productivity preset applied: SolidWorks navigation, zoom at cursor, command palette",
                ),
                6000,
            )

    def _apply_default_preset_if_needed(self) -> None:
        if _PREFERENCES.GetInt("ProductivityPresetVersion") >= _PRODUCTIVITY_PRESET_VERSION:
            return
        QTimer.singleShot(150, self.apply_productivity_preset)

    def _default_entries(
        self,
        recent_commands: list[str],
        recent_workbenches: list[str],
    ) -> list[dict[str, Any]]:
        items: list[dict[str, Any]] = []

        items.extend(self._actions.values())

        current_workbench = _current_workbench_name()
        entry = self._entry_by_id(current_workbench)
        if entry is not None and entry not in items:
            items.append(entry)

        for workbench_name in recent_workbenches:
            entry = self._entry_by_id(workbench_name)
            if entry is not None and entry not in items:
                items.append(entry)

        for command_name in recent_commands:
            entry = self._entry_by_id(command_name)
            if entry is not None and entry not in items:
                items.append(entry)

        for command_name in _DEFAULT_COMMANDS:
            entry = self._entry_by_id(command_name)
            if entry is not None and entry not in items:
                items.append(entry)

        return items[:24]

    def _refresh_runtime_state(self) -> None:
        self._ensure_locale_catalogs_current()
        self._rebuild_actions()
        workbench_name = _current_workbench_name()
        if workbench_name != "":
            self._ensure_workbench_entry(workbench_name)

    def _ensure_palette(self) -> None:
        if self._palette is not None or self._main_window is None:
            return
        self._palette = ProductivityPaletteDialog(self, self._main_window)

    def _ensure_inspector(self) -> None:
        if self._inspector is not None or self._main_window is None:
            return
        self._inspector = SelectionQuickInspectorDock(self, self._main_window)
        self._main_window.addDockWidget(Qt.RightDockWidgetArea, self._inspector)
        self._inspector.visibilityChanged.connect(self._on_inspector_visibility_changed)

    def _ensure_selection_bar(self) -> None:
        if self._selection_bar is None and self._main_window is not None:
            self._selection_bar = SelectionActionBar(self, self._main_window)
        if self._main_window is None or self._main_window_event_filter_installed:
            return
        self._main_window.installEventFilter(self)
        self._main_window_event_filter_installed = True

    def _refresh_theme_chrome(self, force: bool = False) -> None:
        signature = "|".join(str(part) for part in CommandTabTheme.theme_cache_signature())
        if force is False and signature == self._theme_signature:
            return
        self._theme_signature = signature

        if self._palette is not None:
            self._palette.apply_theme()
        if self._inspector is not None:
            self._inspector.apply_theme()
        if self._selection_bar is not None:
            self._selection_bar.apply_theme()

    def _ensure_locale_catalogs_current(self) -> None:
        locale_signature = _current_locale_signature()
        if locale_signature == self._locale_signature:
            return

        self._locale_signature = locale_signature
        self._rebuild_actions()
        self._commands = {}
        self._workbenches = {}
        self._command_catalog_built = False
        self._workbench_catalog_built = False
        self._action_tracking_installed = False

    def _ensure_full_command_catalog(self) -> None:
        if self._command_catalog_built:
            return
        self._rebuild_command_catalog()
        self._command_catalog_built = True
        self._attach_action_tracking()

    def _ensure_full_workbench_catalog(self) -> None:
        if self._workbench_catalog_built:
            return
        self._rebuild_workbench_catalog()
        self._workbench_catalog_built = True

    def _build_command_entry(self, command_name: str) -> dict[str, Any] | None:
        command = _command(command_name)
        if command is None:
            return None

        try:
            raw_info = dict(command.getInfo())
        except Exception:
            raw_info = {}

        info = _localized_command_info(command_name)
        title = str(
            info.get("DisplayText")
            or info.get("ActionText")
            or info.get("menuText")
            or raw_info.get("menuText")
            or command_name
        ).replace("&", "").strip()
        tooltip = str(info.get("toolTip") or raw_info.get("toolTip") or "").strip()
        status_tip = str(info.get("statusTip") or raw_info.get("statusTip") or tooltip).strip()
        shortcut = str(raw_info.get("shortcut") or "").strip()
        icon = str(info.get("pixmap") or raw_info.get("pixmap") or "").strip()

        if title == "" and tooltip == "":
            return None

        subtitle = tooltip if tooltip != title else ""
        return {
            "kind": "command",
            "id": command_name,
            "idLower": command_name.lower(),
            "title": title,
            "titleLower": title.lower(),
            "subtitle": subtitle,
            "shortcut": shortcut,
            "search": " ".join(
                [
                    command_name,
                    title,
                    subtitle,
                    shortcut,
                    status_tip,
                ]
            ).lower(),
            "icon": icon,
        }

    def _ensure_command_entry(self, command_name: str) -> dict[str, Any] | None:
        if command_name in self._commands:
            return self._commands[command_name]
        entry = self._build_command_entry(command_name)
        if entry is None:
            return None
        self._commands[command_name] = entry
        return entry

    def _build_workbench_entry(self, workbench_name: str) -> dict[str, Any] | None:
        if workbench_name in ["", "NoneWorkbench"]:
            return None
        try:
            Gui.getWorkbench(workbench_name)
        except Exception:
            return None
        title = _workbench_title(workbench_name)
        subtitle = _command_display_title("Std_Workbench")
        if subtitle == "":
            subtitle = translate("FreeCAD CommandTab", "Switch workbench")

        return {
            "kind": "workbench",
            "id": workbench_name,
            "idLower": workbench_name.lower(),
            "title": title,
            "titleLower": title.lower(),
            "subtitle": subtitle,
            "shortcut": "",
            "search": f"{workbench_name} {title} workbench switch".lower(),
            "icon": "",
        }

    def _ensure_workbench_entry(self, workbench_name: str) -> dict[str, Any] | None:
        if workbench_name in self._workbenches:
            return self._workbenches[workbench_name]
        entry = self._build_workbench_entry(workbench_name)
        if entry is None:
            return None
        self._workbenches[workbench_name] = entry
        return entry

    def _install_shortcuts(self) -> None:
        if self._main_window is None:
            return

        palette_shortcut = _PREFERENCES.GetString("ProductivityPaletteShortcut") or "Ctrl+Space"
        preset_shortcut = _PREFERENCES.GetString("ProductivityPresetShortcut") or "Ctrl+Alt+P"
        inspector_shortcut = _PREFERENCES.GetString("ProductivityInspectorShortcut") or "Ctrl+Shift+I"
        selection_bar_shortcut = _PREFERENCES.GetString("SelectionActionBarShortcut") or "Ctrl+Shift+A"

        self._palette_shortcut = QShortcut(QKeySequence(palette_shortcut), self._main_window)
        self._palette_shortcut.setContext(Qt.ApplicationShortcut)
        self._palette_shortcut.activated.connect(self.show_palette)

        self._preset_shortcut = QShortcut(QKeySequence(preset_shortcut), self._main_window)
        self._preset_shortcut.setContext(Qt.ApplicationShortcut)
        self._preset_shortcut.activated.connect(self.apply_productivity_preset)

        self._inspector_shortcut = QShortcut(QKeySequence(inspector_shortcut), self._main_window)
        self._inspector_shortcut.setContext(Qt.ApplicationShortcut)
        self._inspector_shortcut.activated.connect(self.toggle_quick_inspector)

        self._selection_bar_shortcut = QShortcut(
            QKeySequence(selection_bar_shortcut), self._main_window
        )
        self._selection_bar_shortcut.setContext(Qt.ApplicationShortcut)
        self._selection_bar_shortcut.activated.connect(self.toggle_selection_action_bar)

    def _rebuild_actions(self) -> None:
        self._actions = {
            "apply_productivity_preset": {
                "kind": "action",
                "id": "apply_productivity_preset",
                "idLower": "apply_productivity_preset",
                "title": translate("FreeCAD CommandTab", "Apply SolidWorks / Fusion Preset"),
                "titleLower": translate(
                    "FreeCAD CommandTab", "Apply SolidWorks / Fusion Preset"
                ).lower(),
                "subtitle": translate(
                    "FreeCAD CommandTab",
                    "Navigation, zoom-at-cursor, palette shortcut",
                ),
                "shortcut": _PREFERENCES.GetString("ProductivityPresetShortcut") or "Ctrl+Alt+P",
                "search": "apply preset ergonomics productivity fusion solidworks navigation shortcut palette",
                "icon": _ICON_PRODUCTIVITY_PRESET,
            },
            "toggle_quick_inspector": {
                "kind": "action",
                "id": "toggle_quick_inspector",
                "idLower": "toggle_quick_inspector",
                "title": translate("FreeCAD CommandTab", "Toggle Quick Inspector"),
                "titleLower": translate("FreeCAD CommandTab", "Toggle Quick Inspector").lower(),
                "subtitle": translate(
                    "FreeCAD CommandTab",
                    "Selection actions, label, visibility, placement and key dimensions",
                ),
                "shortcut": _PREFERENCES.GetString("ProductivityInspectorShortcut") or "Ctrl+Shift+I",
                "search": "toggle quick inspector selection properties visibility placement dimensions edit dock",
                "icon": _ICON_QUICK_INSPECTOR,
            },
            "toggle_selection_action_bar": {
                "kind": "action",
                "id": "toggle_selection_action_bar",
                "idLower": "toggle_selection_action_bar",
                "title": translate("FreeCAD CommandTab", "Toggle Selection Action Bar"),
                "titleLower": translate("FreeCAD CommandTab", "Toggle Selection Action Bar").lower(),
                "subtitle": translate(
                    "FreeCAD CommandTab",
                    "Floating context actions near the workspace for the current selection",
                ),
                "shortcut": _PREFERENCES.GetString("SelectionActionBarShortcut") or "Ctrl+Shift+A",
                "search": "toggle selection action bar floating toolbar context commands quick workspace",
                "icon": _ICON_SELECTION_ACTION_BAR,
            }
        }

    def _build_workbench_command_index(self) -> dict[str, str]:
        """Returns {command_name: workbench_title} for all installed workbenches."""
        index: dict[str, str] = {}
        try:
            for wb_name in Gui.listWorkbenches():
                try:
                    wb = Gui.getWorkbench(wb_name)
                    title = _workbench_title(wb_name)
                    for items in wb.getToolbarItems().values():
                        for cmd in items:
                            if cmd and str(cmd).strip() and cmd not in index:
                                index[str(cmd).strip()] = title
                except Exception:
                    continue
        except Exception:
            pass
        return index

    def _rebuild_command_catalog(self) -> None:
        entries: dict[str, dict[str, Any]] = {}
        wb_index = self._build_workbench_command_index()

        try:
            command_names = sorted(Gui.listCommands())
        except Exception:
            command_names = []

        for command_name in command_names:
            entry = self._build_command_entry(command_name)
            if entry is not None:
                wb_title = wb_index.get(command_name, "")
                if wb_title:
                    entry["workbench"] = wb_title
                    entry["search"] = entry["search"] + " " + wb_title.lower()
                entries[command_name] = entry

        self._commands = entries

    def _rebuild_workbench_catalog(self) -> None:
        entries: dict[str, dict[str, Any]] = {}

        try:
            workbenches = dict(Gui.listWorkbenches())
        except Exception:
            workbenches = {}

        for workbench_name in sorted(workbenches):
            entry = self._build_workbench_entry(workbench_name)
            if entry is not None:
                entries[workbench_name] = entry

        self._workbenches = entries

    def _attach_action_tracking(self) -> None:
        if self._action_tracking_installed:
            return
        for command_name in self._commands:
            command = _command(command_name)
            if command is None:
                continue

            try:
                action = command.getAction()
            except Exception:
                action = None

            if action is None:
                continue

            if bool(action.property("FreeCADCommandTabProductivityTracked")):
                continue

            action.setProperty("FreeCADCommandTabProductivityTracked", True)
            action.setProperty("FreeCADCommandTabProductivityCommandName", command_name)
            action.triggered.connect(self._on_command_triggered)
        self._action_tracking_installed = True

    def _install_selection_observer(self) -> None:
        if self._selection_observer_installed:
            return
        try:
            Gui.Selection.addObserver(self, 0)
            self._selection_observer_installed = True
        except Exception:
            self._selection_observer_installed = False

    def _restore_inspector_visibility(self) -> None:
        if _PREFERENCES.GetInt("QuickInspectorVersion") < _QUICK_INSPECTOR_VERSION:
            self._ensure_inspector()
            if self._inspector is None:
                return
            self._inspector.setVisible(True)
            _PREFERENCES.SetInt("QuickInspectorVersion", _QUICK_INSPECTOR_VERSION)
            _PREFERENCES.SetBool("QuickInspectorVisible", True)
            App.saveParameter()
            return

        if _PREFERENCES.GetBool("QuickInspectorVisible") is False:
            return
        self._ensure_inspector()
        if self._inspector is None:
            return
        self._inspector.setVisible(_PREFERENCES.GetBool("QuickInspectorVisible"))

    def _restore_selection_bar_enabled(self) -> None:
        if _PREFERENCES.GetInt("SelectionActionBarVersion") < _SELECTION_ACTION_BAR_VERSION:
            _PREFERENCES.SetInt("SelectionActionBarVersion", _SELECTION_ACTION_BAR_VERSION)
            _PREFERENCES.SetBool("SelectionActionBarEnabled", True)
            App.saveParameter()

    def _on_inspector_visibility_changed(self, visible: bool) -> None:
        _PREFERENCES.SetBool("QuickInspectorVisible", bool(visible))
        App.saveParameter()
        if visible:
            self._schedule_selection_refresh()

    def _schedule_selection_refresh(self) -> None:
        if self._selection_refresh_timer is None:
            return
        self._selection_refresh_timer.start(40)

    def _refresh_selection_state(self) -> None:
        inspector_visible = self._inspector is not None and self._inspector.isVisible()
        selection_bar_enabled = _PREFERENCES.GetBool("SelectionActionBarEnabled")
        if inspector_visible is False and selection_bar_enabled is False:
            return
        objects = _selected_objects()
        action_entries = self._selection_action_entries(objects)
        if inspector_visible and self._inspector is not None:
            self._inspector.refresh_selection(objects, action_entries)
        if selection_bar_enabled and len(objects) > 0:
            self._ensure_selection_bar()
        if self._selection_bar is not None:
            self._selection_bar.set_state(
                objects,
                action_entries,
                selection_bar_enabled,
            )

    def _entry_by_id(self, identifier: str) -> dict[str, Any] | None:
        if identifier in self._actions:
            return dict(self._actions[identifier])
        if identifier in self._commands:
            return dict(self._commands[identifier])
        entry = self._ensure_command_entry(identifier)
        if entry is not None:
            return dict(entry)
        entry = self._ensure_workbench_entry(identifier)
        if entry is not None:
            return dict(entry)
        return None

    def _selection_action_entries(self, objects: list[Any]) -> list[dict[str, Any]]:
        action_ids: list[str] = []
        if len(objects) == 0:
            action_ids = [
                "Std_ViewFitAll",
                "CommandTab_CommandPalette",
                "toggle_quick_inspector",
                "toggle_selection_action_bar",
                "apply_productivity_preset",
            ]
        else:
            action_ids.extend(
                [
                    "Std_ViewFitSelection",
                    "Std_ToggleVisibility",
                    "Std_TransformManip",
                ]
            )
            if len(objects) == 1:
                obj = objects[0]
                type_id = str(getattr(obj, "TypeId", ""))
                if "Sketcher::SketchObject" in type_id:
                    action_ids = ["Sketcher_EditSketch", "Sketcher_ValidateSketch"] + action_ids
                if "PartDesign::Body" in type_id or "PartDesign::Feature" in type_id:
                    action_ids = ["PartDesign_NewSketch", "PartDesign_Pad", "PartDesign_Pocket"] + action_ids
                if "Assembly::" in type_id:
                    action_ids = ["Assembly_CreateJoint"] + action_ids
                action_ids.extend(
                    ["Std_Appearance", "Std_DuplicateSelection", "toggle_quick_inspector"]
                )
            action_ids.append("Std_Delete")

        entries: list[dict[str, Any]] = []
        seen: set[str] = set()
        for identifier in action_ids:
            entry = self._entry_by_id(identifier)
            if entry is None:
                continue
            if entry["id"] in seen:
                continue
            seen.add(entry["id"])
            entries.append(entry)
        return entries

    def _run_command(self, command_name: str) -> None:
        self._record_recent("ProductivityRecentCommands", command_name)
        try:
            Gui.runCommand(command_name, 0)
        except Exception:
            try:
                Gui.runCommand(command_name)
            except Exception:
                pass

    def _record_recent(self, setting_name: str, value: str) -> None:
        if value in [
            "CommandTab_CommandPalette",
            "CommandTab_ApplyProductivityPreset",
            "CommandTab_ToggleQuickInspector",
            "CommandTab_ToggleSelectionActionBar",
        ]:
            return

        values = _load_string_list(setting_name)
        if value in values:
            values.remove(value)
        values.insert(0, value)
        _save_string_list(setting_name, values)

    def _on_command_triggered(self, _checked=False) -> None:
        try:
            action = self.sender()
        except Exception:
            action = None

        if action is None:
            return

        command_name = str(action.property("FreeCADCommandTabProductivityCommandName") or "").strip()
        if command_name == "":
            return
        self._record_recent("ProductivityRecentCommands", command_name)

    def _refresh_current_wb_commands(self) -> None:
        workbench_name = _current_workbench_name()
        if not workbench_name:
            self._current_wb_commands = set()
            return
        try:
            wb = Gui.getWorkbench(workbench_name)
            toolbar_items = wb.getToolbarItems()
            commands: set[str] = set()
            for items in toolbar_items.values():
                for cmd in items:
                    if cmd and str(cmd).strip():
                        commands.add(str(cmd).strip())
            self._current_wb_commands = commands
        except Exception:
            self._current_wb_commands = set()

    def _on_workbench_activated(self, *_args) -> None:
        self._ensure_locale_catalogs_current()
        workbench_name = _current_workbench_name()
        if workbench_name != "":
            self._record_recent("ProductivityRecentWorkbenches", workbench_name)
            self._ensure_workbench_entry(workbench_name)
        self._refresh_current_wb_commands()
        self._rebuild_actions()
        self._refresh_theme_chrome()
        self._schedule_selection_refresh()

    def eventFilter(self, watched, event):
        if watched is self._main_window and event is not None:
            if event.type() in [
                QEvent.Resize,
                QEvent.Move,
                QEvent.Show,
                QEvent.WindowStateChange,
            ]:
                if self._selection_bar is not None:
                    QTimer.singleShot(0, self._selection_bar.sync_position)
            if event.type() in [
                QEvent.StyleChange,
                QEvent.PaletteChange,
                QEvent.ApplicationPaletteChange,
            ]:
                QTimer.singleShot(0, self._refresh_theme_chrome)
            if event.type() in [QEvent.LanguageChange, QEvent.LocaleChange]:
                QTimer.singleShot(0, self._ensure_locale_catalogs_current)
        return QObject.eventFilter(self, watched, event)

    def addSelection(self, *_args) -> None:
        self._schedule_selection_refresh()

    def removeSelection(self, *_args) -> None:
        self._schedule_selection_refresh()

    def setSelection(self, *_args) -> None:
        self._schedule_selection_refresh()

    def clearSelection(self, *_args) -> None:
        self._schedule_selection_refresh()

    def pickedListChanged(self) -> None:
        self._schedule_selection_refresh()


class CommandTabCommandPaletteCommand:
    def GetResources(self):
        return {
            "Pixmap": "./Resources/icons/view-select.svg",
            "Accel": "Ctrl+Space",
            "MenuText": translate("FreeCAD CommandTab", "Command Palette"),
            "ToolTip": translate(
                "FreeCAD CommandTab",
                "Search commands, workbenches and ergonomic actions",
            ),
        }

    def Activated(self):
        show_command_palette()

    def IsActive(self):
        return Gui.getMainWindow() is not None


class CommandTabApplyProductivityPresetCommand:
    def GetResources(self):
        return {
            "Pixmap": "./Resources/icons/align-to-selection.svg",
            "Accel": "Ctrl+Alt+P",
            "MenuText": translate("FreeCAD CommandTab", "Apply Productivity Preset"),
            "ToolTip": translate(
                "FreeCAD CommandTab",
                "Apply a SolidWorks/Fusion-style navigation and productivity preset",
            ),
        }

    def Activated(self):
        apply_productivity_preset()

    def IsActive(self):
        return Gui.getMainWindow() is not None


class CommandTabToggleQuickInspectorCommand:
    def GetResources(self):
        return {
            "Pixmap": "./Resources/icons/view-measurement.svg",
            "Accel": "Ctrl+Shift+I",
            "MenuText": translate("FreeCAD CommandTab", "Toggle Quick Inspector"),
            "ToolTip": translate(
                "FreeCAD CommandTab",
                "Show a dock with selection actions, label, visibility and editable dimensions",
            ),
        }

    def Activated(self):
        toggle_quick_inspector()

    def IsActive(self):
        return Gui.getMainWindow() is not None


class CommandTabToggleSelectionActionBarCommand:
    def GetResources(self):
        return {
            "Pixmap": "./Resources/icons/view-select.svg",
            "Accel": "Ctrl+Shift+A",
            "MenuText": translate("FreeCAD CommandTab", "Toggle Selection Action Bar"),
            "ToolTip": translate(
                "FreeCAD CommandTab",
                "Show or hide the floating contextual action bar for the current selection",
            ),
        }

    def Activated(self):
        toggle_selection_action_bar()

    def IsActive(self):
        return Gui.getMainWindow() is not None


def ensure_productivity_controller() -> ProductivityController:
    global _CONTROLLER

    if _CONTROLLER is None:
        _CONTROLLER = ProductivityController()
    return _CONTROLLER


def install_productivity_layer() -> bool:
    controller = ensure_productivity_controller()
    return controller.install()


def show_command_palette() -> None:
    controller = ensure_productivity_controller()
    controller.show_palette()


def apply_productivity_preset() -> None:
    controller = ensure_productivity_controller()
    if controller.install():
        controller.apply_productivity_preset()


def toggle_quick_inspector() -> None:
    controller = ensure_productivity_controller()
    if controller.install():
        controller.toggle_quick_inspector()


def toggle_selection_action_bar() -> None:
    controller = ensure_productivity_controller()
    if controller.install():
        controller.toggle_selection_action_bar()


Gui.addCommand("CommandTab_CommandPalette", CommandTabCommandPaletteCommand())
Gui.addCommand("CommandTab_ApplyProductivityPreset", CommandTabApplyProductivityPresetCommand())
Gui.addCommand("CommandTab_ToggleQuickInspector", CommandTabToggleQuickInspectorCommand())
Gui.addCommand("CommandTab_ToggleSelectionActionBar", CommandTabToggleSelectionActionBarCommand())
