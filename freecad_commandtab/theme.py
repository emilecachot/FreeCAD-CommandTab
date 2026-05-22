from __future__ import annotations

from pathlib import Path

import FreeCAD as App
import FreeCADGui as Gui
from PySide.QtGui import QColor

from freecad_commandtab import paths

_MAIN_WINDOW_PREFERENCES = App.ParamGet("User parameter:BaseApp/Preferences/MainWindow")
_COMMANDTAB_PREFERENCES = App.ParamGet("User parameter:BaseApp/Preferences/Mod/FreeCAD-CommandTab")
_THEME_TOKEN_PREFERENCES = App.ParamGet(
    "User parameter:BaseApp/Preferences/Themes/UserTokens"
)
_EMBEDDED_THEME_ROOT = Path(
    paths.addon_path("Resources", "stylesheets", "theme")
)
_LEGACY_THEME_ROOT = Path(paths.addon_path("Theme-main"))
_THEME_ROOT = (
    _EMBEDDED_THEME_ROOT
    if _EMBEDDED_THEME_ROOT.is_dir()
    else _LEGACY_THEME_ROOT
)
_THEME_MODE_KEY = "ThemeBootstrapMode"


def _theme_mode_override() -> str:
    value = _COMMANDTAB_PREFERENCES.GetString("NativeThemeMode").strip().lower()
    if value in ["dark", "light"]:
        return value
    return "auto"


def _current_stylesheet_name() -> str:
    return _MAIN_WINDOW_PREFERENCES.GetString("StyleSheet").strip()


def _stored_theme_mode() -> str:
    stylesheet_name = _current_stylesheet_name().lower()
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
    return ""


def _detect_theme_mode() -> str:
    override = _theme_mode_override()
    if override in ["dark", "light"]:
        return override

    stored_theme_mode = _stored_theme_mode()
    if stored_theme_mode in ["dark", "light"]:
        return stored_theme_mode

    try:
        window = Gui.getMainWindow()
        color = window.palette().window().color()
        return "dark" if color.lightness() < 128 else "light"
    except Exception:
        return "dark"


def _theme_assets() -> dict[str, str]:
    return {
        "root": str(_THEME_ROOT),
        "dark_qss": str(_THEME_ROOT / "OpenDark" / "OpenDark.qss"),
        "light_qss": str(_THEME_ROOT / "OpenLight" / "OpenLight.qss"),
        "dark_overlay_qss": str(
            _THEME_ROOT / "OpenDark" / "overlay" / "OpenDark_Overlay.qss"
        ),
        "light_overlay_qss": str(
            _THEME_ROOT / "OpenLight" / "overlay" / "OpenLight_Overlay.qss"
        ),
        "preferences_cfg": str(
            _THEME_ROOT / "OpenPreferences" / "OpenPreferences.cfg"
        ),
    }


def _dark_tokens() -> dict[str, str | bool]:
    return {
        "mode": "dark",
        "isDark": True,
        "shellBackground": "#1e2329",  # Fond principal du ruban.
        "shellBorder": "#4b5563",  # Bordure globale du conteneur ruban.
        "controlBackground": "#171b21",  # Fond des controls secondaires.
        "panelCardBackground": "#262c34",  # Fond des cartes/panneaux.
        "panelCardBorder": "#4b5563",  # Bordure des cartes/panneaux.
        "panelFooterBackground": "#222830",  # Fond du footer des panneaux.
        "panelFooterBorder": "#3a4452",  # Bordure du footer des panneaux.
        "panelBodyTop": "#323a45",  # Dégradé haut du corps de panneau.
        "panelBodyBottom": "#252c34",  # Dégradé bas du corps de panneau.
        "panelBodyAccent": "#3a8fd8",  # Accent principal des panneaux.
        "panelBodyGlow": "#ffffff",  # Lueur/surbrillance de panneau.
        "titleText": "#d7dde8",  # Texte de titre (sera canoniquement forcé ensuite).
        "secondaryText": "#c2ccd8",  # Texte secondaire.
        "disabledText": "#8a95a3",  # Texte désactivé.
        "separator": "#7a8594",  # Séparateurs visuels.
        "accentBorder": "#2d6ea6",  # Bordure accentuée.
        "quickBackground": "#1e2329",  # Fond barre accès rapide.
        "quickBorder": "#4b5563",  # Bordure barre accès rapide.
        "quickHoverBackground": "#303845",  # Fond hover accès rapide.
        "quickHoverBorder": "#3a8fd8",  # Bordure hover accès rapide.
        "tabBackground": "#272e37",  # Fond onglets inactifs.
        "tabHoverBackground": "#323b46",  # Fond onglets au survol.
        "tabSelectedBackground": "#3b4653",  # Fond onglet sélectionné.
        "tabBorder": "#4b5563",  # Bordure des onglets.
        "tabSelectedBorder": "#6f7e90",  # Bordure onglet sélectionné.
        "tabAccent": "#3a8fd8",  # Accent visuel onglets.
        "buttonIdleTop": "#2a313a",  # Haut bouton au repos.
        "buttonIdleBottom": "#262d36",  # Bas bouton au repos.
        "buttonActiveTop": "#384350",  # Haut bouton actif/hover.
        "buttonActiveBottom": "#313b47",  # Bas bouton actif/hover.
        "buttonPressedTop": "#252d37",  # Haut bouton pressé.
        "buttonPressedBottom": "#212933",  # Bas bouton pressé.
        "buttonBorder": "#4d5868",  # Bordure bouton au repos.
        "buttonActiveBorder": "#4ea3ed",  # Bordure bouton actif.
        "buttonFocusBorder": "#7ec3ff",  # Bordure bouton focus clavier.
        "buttonText": "#d7dde8",  # Texte des boutons.
        "buttonPlate": "#ffffff",  # Plaque interne bouton/icon plate.
        "tooltipBackground": "#2e7cc2",  # Fond tooltip.
        "buttonPlateIdleAlpha": "#0c",  # Alpha plaque bouton au repos.
        "buttonPlateActiveAlpha": "#1e",  # Alpha plaque bouton actif.
        "buttonHighlightAlpha": "#26",  # Alpha de highlight générique.
        "buttonPressedHighlightAlpha": "#14",  # Alpha de highlight pressé.
    }


def _light_tokens() -> dict[str, str | bool]:
    return {
        "mode": "light",
        "isDark": False,
        "shellBackground": "#eef1f5",  # Fond principal du ruban.
        "shellBorder": "#bcc5d1",  # Bordure globale du conteneur ruban.
        "controlBackground": "#f6f8fb",  # Fond des controls secondaires.
        "panelCardBackground": "#ffffff",  # Fond des cartes/panneaux.
        "panelCardBorder": "#d5dce5",  # Bordure des cartes/panneaux.
        "panelFooterBackground": "#f1f5fa",  # Fond du footer des panneaux.
        "panelFooterBorder": "#d4dbe5",  # Bordure du footer des panneaux.
        "panelBodyTop": "#ffffff",  # Dégradé haut du corps de panneau.
        "panelBodyBottom": "#f6f9fc",  # Dégradé bas du corps de panneau.
        "panelBodyAccent": "#2f74b8",  # Accent principal des panneaux.
        "panelBodyGlow": "#ffffff",  # Lueur/surbrillance de panneau.
        "titleText": "#000000",  # Texte de titre (sera canoniquement forcé ensuite).
        "secondaryText": "#1f2937",  # Texte secondaire.
        "disabledText": "#6b7280",  # Texte désactivé.
        "separator": "#8b95a3",  # Séparateurs visuels.
        "accentBorder": "#2d6ea6",  # Bordure accentuée.
        "quickBackground": "#eef1f5",  # Fond barre accès rapide.
        "quickBorder": "#d5dce5",  # Bordure barre accès rapide.
        "quickHoverBackground": "#e8f2fc",  # Fond hover accès rapide.
        "quickHoverBorder": "#2f84d1",  # Bordure hover accès rapide.
        "tabBackground": "#f1f5fa",  # Fond onglets inactifs.
        "tabHoverBackground": "#f8fafd",  # Fond onglets au survol.
        "tabSelectedBackground": "#ffffff",  # Fond onglet sélectionné.
        "tabBorder": "#d4dbe5",  # Bordure des onglets.
        "tabSelectedBorder": "#b5bfcd",  # Bordure onglet sélectionné.
        "tabAccent": "#2f74b8",  # Accent visuel onglets.
        "buttonIdleTop": "#ffffff",  # Haut bouton au repos.
        "buttonIdleBottom": "#f7f9fc",  # Bas bouton au repos.
        "buttonActiveTop": "#edf4fc",  # Haut bouton actif/hover.
        "buttonActiveBottom": "#e1ecf8",  # Bas bouton actif/hover.
        "buttonPressedTop": "#deeaf7",  # Haut bouton pressé.
        "buttonPressedBottom": "#d4e2f2",  # Bas bouton pressé.
        "buttonBorder": "#cad4e1",  # Bordure bouton au repos.
        "buttonActiveBorder": "#3f8ed6",  # Bordure bouton actif.
        "buttonFocusBorder": "#2c76bd",  # Bordure bouton focus clavier.
        "buttonText": "#000000",  # Texte des boutons.
        "buttonPlate": "#fcfdff",  # Plaque interne bouton/icon plate.
        "tooltipBackground": "#d9ebff",  # Fond tooltip.
        "buttonPlateIdleAlpha": "#ff",  # Alpha plaque bouton au repos.
        "buttonPlateActiveAlpha": "#ff",  # Alpha plaque bouton actif.
        "buttonHighlightAlpha": "#14",  # Alpha de highlight générique.
        "buttonPressedHighlightAlpha": "#0a",  # Alpha de highlight pressé.
    }


def _color_name(color: QColor) -> str:
    if color.isValid() is False:
        return "#000000"  # Fallback de sérialisation si une couleur est invalide.
    return color.name()


def _is_usable_color(color: QColor) -> bool:
    return color.isValid() and color.alpha() > 0


def _first_valid_color(*colors: QColor) -> QColor:
    for color in colors:
        if _is_usable_color(color):
            return color
    return QColor()


def _style_mapping_color(name: str) -> QColor:
    try:
        import StyleMapping_CommandTab as StyleMappingCommandTab

        color = QColor(StyleMappingCommandTab.ReturnStyleItem(name, IgnoreOverlay=True))
        if _is_usable_color(color):
            return color
    except Exception:
        pass
    return QColor()


def _theme_user_token_color(name: str) -> QColor:
    try:
        color = QColor.fromRgba(_THEME_TOKEN_PREFERENCES.GetUnsigned(name))
        if _is_usable_color(color):
            return color
    except Exception:
        pass
    return QColor()


def _shade(color: QColor, factor: int) -> QColor:
    if color.isValid() is False:
        return QColor()
    return color.lighter(factor) if factor >= 100 else color.darker(200 - factor)


def _channel_luminance(value: float) -> float:
    normalized = value / 255.0
    if normalized <= 0.03928:
        return normalized / 12.92
    return ((normalized + 0.055) / 1.055) ** 2.4


def _relative_luminance(color: QColor) -> float:
    if color.isValid() is False:
        return 0.0
    return (
        0.2126 * _channel_luminance(color.red())
        + 0.7152 * _channel_luminance(color.green())
        + 0.0722 * _channel_luminance(color.blue())
    )


def _contrast_ratio(foreground: QColor, background: QColor) -> float:
    lighter = max(_relative_luminance(foreground), _relative_luminance(background))
    darker = min(_relative_luminance(foreground), _relative_luminance(background))
    return (lighter + 0.05) / (darker + 0.05)


def _accessible_text_color(
    preferred: QColor, background: QColor, *, minimum_ratio: float = 4.5
) -> QColor:
    fallback_light = QColor("#f8f9fa")  # Candidat texte clair.
    fallback_dark = QColor("#212529")  # Candidat texte sombre.

    candidates = []
    if preferred.isValid():
        candidates.append(preferred)
    candidates.extend([fallback_light, fallback_dark])

    best = candidates[0]
    best_ratio = -1.0
    for candidate in candidates:
        ratio = _contrast_ratio(candidate, background)
        if candidate == preferred and ratio >= minimum_ratio:
            return candidate
        if ratio > best_ratio:
            best = candidate
            best_ratio = ratio

    return best


def _build_tokens_from_seed(
    mode: str,
    *,
    shell_background: QColor,
    hover: QColor,
    border: QColor,
    title_text: QColor,
    accent: QColor,
    panel_card_background: QColor = None,
    panel_footer_background: QColor = None,
    panel_body_top: QColor = None,
    panel_body_bottom: QColor = None,
    quick_background: QColor = None,
    tab_background: QColor = None,
    tab_selected_background: QColor = None,
    button_text: QColor = None,
    button_plate: QColor = None,
) -> dict[str, str | bool]:
    if panel_card_background is None:
        panel_card_background = QColor()
    if panel_footer_background is None:
        panel_footer_background = QColor()
    if panel_body_top is None:
        panel_body_top = QColor()
    if panel_body_bottom is None:
        panel_body_bottom = QColor()
    if quick_background is None:
        quick_background = QColor()
    if tab_background is None:
        tab_background = QColor()
    if tab_selected_background is None:
        tab_selected_background = QColor()
    if button_text is None:
        button_text = QColor()
    if button_plate is None:
        button_plate = QColor()

    fallback_tokens = _dark_tokens() if mode == "dark" else _light_tokens()
    is_dark = mode == "dark"

    fallback_shell = QColor(str(fallback_tokens["shellBackground"]))
    fallback_hover = QColor(str(fallback_tokens["quickHoverBackground"]))
    fallback_border = QColor(str(fallback_tokens["shellBorder"]))
    fallback_title = QColor(str(fallback_tokens["titleText"]))
    fallback_accent = QColor(str(fallback_tokens["tabAccent"]))

    shell_background = _first_valid_color(shell_background, fallback_shell)
    accent = _first_valid_color(accent, hover, fallback_accent)
    border = _first_valid_color(border, fallback_border)
    hover = _first_valid_color(
        hover,
        _shade(accent, 110 if is_dark else 102),
        fallback_hover,
    )
    panel_card_background = _first_valid_color(
        panel_card_background,
        _shade(shell_background, 118 if is_dark else 106),
        QColor(str(fallback_tokens["panelCardBackground"])),
    )
    panel_footer_background = _first_valid_color(
        panel_footer_background,
        _shade(panel_card_background, 94 if is_dark else 96),
        QColor(str(fallback_tokens["panelFooterBackground"])),
    )
    panel_body_top = _first_valid_color(
        panel_body_top,
        _shade(panel_card_background, 118 if is_dark else 104),
        QColor(str(fallback_tokens["panelBodyTop"])),
    )
    panel_body_bottom = _first_valid_color(
        panel_body_bottom,
        _shade(panel_card_background, 96 if is_dark else 98),
        QColor(str(fallback_tokens["panelBodyBottom"])),
    )
    quick_background = _first_valid_color(
        quick_background,
        shell_background,
        QColor(str(fallback_tokens["quickBackground"])),
    )
    tab_background = _first_valid_color(
        tab_background,
        panel_footer_background,
        QColor(str(fallback_tokens["tabBackground"])),
    )
    tab_selected_background = _first_valid_color(
        tab_selected_background,
        panel_body_top,
        QColor(str(fallback_tokens["tabSelectedBackground"])),
    )
    button_plate = _first_valid_color(
        button_plate,
        panel_body_top,
        QColor(str(fallback_tokens["buttonPlate"])),
    )

    title_text = _accessible_text_color(
        _first_valid_color(title_text, fallback_title),
        panel_footer_background,
    )
    button_text = _accessible_text_color(
        _first_valid_color(button_text, title_text),
        panel_card_background,
    )
    secondary_text = _first_valid_color(
        QColor(str(fallback_tokens["secondaryText"])),
        _shade(title_text, 76 if is_dark else 132),
    )
    disabled_text = _first_valid_color(
        QColor(str(fallback_tokens["disabledText"])),
        _shade(border, 145 if is_dark else 112),
    )
    control_background = _first_valid_color(
        QColor(str(fallback_tokens["controlBackground"])),
        panel_body_bottom,
        panel_footer_background,
    )
    accent_border = _first_valid_color(
        QColor(str(fallback_tokens["accentBorder"])),
        _shade(accent, 138 if is_dark else 108),
    )
    tooltip_background = _first_valid_color(
        QColor(str(fallback_tokens["tooltipBackground"])),
        _shade(accent, 82 if is_dark else 112),
    )

    return {
        "mode": mode,
        "isDark": is_dark,
        "shellBackground": _color_name(shell_background),
        "shellBorder": _color_name(border),
        "controlBackground": _color_name(control_background),
        "panelCardBackground": _color_name(panel_card_background),
        "panelCardBorder": _color_name(border),
        "panelFooterBackground": _color_name(panel_footer_background),
        "panelFooterBorder": _color_name(_shade(border, 110 if is_dark else 96)),
        "panelBodyTop": _color_name(panel_body_top),
        "panelBodyBottom": _color_name(panel_body_bottom),
        "panelBodyAccent": _color_name(accent),
        "panelBodyGlow": "#ffffff",  # Lueur blanche neutre des panneaux.
        "titleText": _color_name(title_text),
        "secondaryText": _color_name(secondary_text),
        "disabledText": _color_name(disabled_text),
        "separator": _color_name(_shade(border, 108 if is_dark else 94)),
        "accentBorder": _color_name(accent_border),
        "quickBackground": _color_name(quick_background),
        "quickBorder": _color_name(border),
        "quickHoverBackground": _color_name(hover),
        "quickHoverBorder": _color_name(accent),
        "tabBackground": _color_name(tab_background),
        "tabHoverBackground": _color_name(
            _shade(hover, 106 if is_dark else 101)
        ),
        "tabSelectedBackground": _color_name(tab_selected_background),
        "tabBorder": _color_name(border),
        "tabSelectedBorder": _color_name(_shade(accent, 118 if is_dark else 92)),
        "tabAccent": _color_name(accent),
        "buttonIdleTop": _color_name(panel_card_background),
        "buttonIdleBottom": _color_name(_shade(panel_card_background, 98 if is_dark else 99)),
        "buttonActiveTop": _color_name(_shade(accent, 110 if is_dark else 98)),
        "buttonActiveBottom": _color_name(_shade(accent, 92 if is_dark else 94)),
        "buttonPressedTop": _color_name(_shade(accent, 94 if is_dark else 92)),
        "buttonPressedBottom": _color_name(_shade(accent, 82 if is_dark else 88)),
        "buttonBorder": _color_name(border),
        "buttonActiveBorder": _color_name(accent),
        "buttonFocusBorder": _color_name(_shade(accent, 120)),
        "buttonText": _color_name(button_text),
        "buttonPlate": _color_name(button_plate),
        "tooltipBackground": _color_name(tooltip_background),
        "buttonPlateIdleAlpha": "#0c" if is_dark else "#ff",  # Alpha plaque bouton au repos.
        "buttonPlateActiveAlpha": "#1e" if is_dark else "#ff",  # Alpha plaque bouton actif.
        "buttonHighlightAlpha": "#26" if is_dark else "#14",  # Alpha highlight bouton.
        "buttonPressedHighlightAlpha": "#14" if is_dark else "#0a",  # Alpha highlight pressé.
    }


def _palette_tokens(mode: str) -> dict[str, str | bool] | None:
    try:
        window = Gui.getMainWindow()
        palette = window.palette()
    except Exception:
        palette = None

    shell_background = _first_valid_color(
        _style_mapping_color("Background_Color"),
        _theme_user_token_color("GeneralBackgroundColor"),
        palette.window().color() if palette is not None else QColor(),
    )
    hover = _first_valid_color(
        _style_mapping_color("Background_Color_Hover"),
        _theme_user_token_color("GeneralBackgroundHoverColor"),
        palette.highlight().color() if palette is not None else QColor(),
    )
    border = _first_valid_color(
        _style_mapping_color("Border_Color"),
        _theme_user_token_color("GeneralBorderColor"),
        _theme_user_token_color("GeneralBorderHoverColor"),
        palette.mid().color() if palette is not None else QColor(),
        palette.dark().color() if palette is not None else QColor(),
    )
    title_text = _first_valid_color(
        _style_mapping_color("FontColor"),
        _theme_user_token_color("TextForegroundColor"),
        palette.windowText().color() if palette is not None else QColor(),
    )
    accent = _first_valid_color(
        _style_mapping_color("ApplicationButton_Background"),
        palette.highlight().color() if palette is not None else QColor(),
        hover,
    )

    if (
        _is_usable_color(shell_background) is False
        and _is_usable_color(border) is False
        and _is_usable_color(title_text) is False
        and _is_usable_color(accent) is False
    ):
        return None

    return _build_tokens_from_seed(
        mode,
        shell_background=shell_background,
        hover=hover,
        border=border,
        title_text=title_text,
        accent=accent,
        panel_card_background=palette.button().color() if palette is not None else QColor(),
        panel_footer_background=palette.alternateBase().color() if palette is not None else QColor(),
        panel_body_top=palette.button().color() if palette is not None else QColor(),
        panel_body_bottom=palette.base().color() if palette is not None else QColor(),
        quick_background=palette.button().color() if palette is not None else QColor(),
        tab_background=palette.button().color() if palette is not None else QColor(),
        tab_selected_background=palette.base().color() if palette is not None else QColor(),
        button_text=palette.buttonText().color() if palette is not None else QColor(),
        button_plate=palette.base().color() if palette is not None else QColor(),
    )


def current_theme_tokens() -> dict[str, str | bool | dict[str, str]]:
    mode = _detect_theme_mode()
    use_theme_tokens = _THEME_ROOT.is_dir() and (
        "openlight" in _current_stylesheet_name().lower()
        or "opendark" in _current_stylesheet_name().lower()
        or _stored_theme_mode() in ["dark", "light"]
    )

    tokens = _palette_tokens(mode)
    if tokens is None:
        tokens = _dark_tokens() if mode == "dark" else _light_tokens()
    canonical_text = primary_text_color()
    tokens["mode"] = mode
    tokens["isDark"] = bool(mode == "dark")
    tokens["titleText"] = canonical_text
    tokens["buttonText"] = canonical_text
    tokens["secondaryText"] = canonical_text
    tokens["disabledText"] = canonical_text
    tokens["theme"] = _theme_assets()
    tokens["themeIntegrated"] = bool(use_theme_tokens)
    tokens["themeMode"] = _stored_theme_mode() or mode
    tokens["styleSheetName"] = _current_stylesheet_name()
    return tokens


def primary_text_color() -> str:
    """Return the canonical addon text color used by commandtab controls."""
    mode = _detect_theme_mode()
    return "#000000" if mode == "light" else "#d7dde8"  # Light: noir, Dark: gris clair.


def theme_cache_signature() -> tuple[str, str, str, str]:
    tokens = current_theme_tokens()
    return (
        "|".join(
            [
                str(tokens.get("mode", "dark")),
                str(tokens.get("styleSheetName", "")),
                str(tokens.get("shellBackground", "")),
                str(tokens.get("panelCardBackground", "")),
                str(tokens.get("tabAccent", "")),
                str(tokens.get("titleText", "")),
            ]
        ),
        str(_COMMANDTAB_PREFERENCES.GetString("IconThemeMode").strip().lower() or "modern"),
        "external"
        if bool(_COMMANDTAB_PREFERENCES.GetBool("UseExternalIconThemeFallback"))
        else "bundled",
        str(tokens.get("themeMode", "")),
    )
