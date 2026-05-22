# *************************************************************************
# *                                                                       *
# * Copyright (c) 2019-2024 Paul Ebbers                                   *
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
import os
import sys

import FreeCAD as App
import FreeCADGui as Gui
from PySide.QtGui import QColor, QIcon, QPixmap
from PySide.QtWidgets import (
        QMainWindow,
        QToolButton,
)

import Parameters_CommandTab
import Standard_Functions_CommandTab as StandardFunctions

# Get the resources
pathIcons = Parameters_CommandTab.ICON_LOCATION
pathStylSheets = Parameters_CommandTab.STYLESHEET_LOCATION
pathUI = Parameters_CommandTab.UI_LOCATION
pathBackup = Parameters_CommandTab.BACKUP_LOCATION
sys.path.append(pathIcons)
sys.path.append(pathStylSheets)
sys.path.append(pathUI)
sys.path.append(pathBackup)

mw: QMainWindow = Gui.getMainWindow()

preferences = App.ParamGet('User parameter:BaseApp/Preferences/Themes/UserTokens/')
BackGroundColor = preferences.GetString('GeneralBackgroundColor')
HoverColor = preferences.GetString('GeneralBackgroundHoverColor')
BorderColor = preferences.GetString('GeneralBorderColor')
BorderColorHover = preferences.GetString('GeneralBorderHoverColor')
TextColor = preferences.GetString('TextForegroundColor')

btn = QToolButton()


def GetColorSetting(settingName: str) -> object:
        # Create a tuple from the int value of the color
        result = QColor.fromRgba(preferences.GetUnsigned(settingName)).toTuple()

        # correct the order of the tuple
        result = (result[3], result[0], result[1], result[2])

        return result

def DarkMode():
    import os
    import xml.etree.ElementTree as ET

    # Define the standard result
    IsDarkTheme = False

    # Get the current stylesheet for FreeCAD
    FreeCAD_preferences = App.ParamGet("User parameter:BaseApp/Preferences/MainWindow")
    currentStyleSheet = FreeCAD_preferences.GetString("StyleSheet")
    currentTheme = FreeCAD_preferences.GetString("Theme")
    currentOverlay = FreeCAD_preferences.GetString("OverlayActiveStyleSheet")

    # if no stylesheet is selected return
    if currentStyleSheet is None or currentStyleSheet == "":
        return

    # FreeCAD Dark is part of FreeCAD, so set the result to True manually
    if (
        currentStyleSheet.lower() == "freecad dark.qss"
        or currentTheme.lower() == "freecad dark"
        or "dark theme" in currentOverlay.lower()
    ):
        return True

    # OpenLight and OpenDark are from one addon. Set the currentStyleSheet value to the addon folder
    if "OpenLight.qss" in currentStyleSheet:
        return False
    if "OpenDark.qss" in currentStyleSheet:
        return True

    path = os.path.dirname(__file__)
    # Get the folder with add-ons
    for _i in range(2):
        # Starting point
        path = os.path.dirname(path)

    # Go through the sub-folders
    for _root, dirs, _files in os.walk(path):
        for name in dirs:

            # if the current stylesheet matches a sub directory, try to get the package.xml
            if currentStyleSheet.replace(".qss", "").lower() in name.lower():
                try:
                    packageXML = os.path.join(path, name, "package.xml")

                    # Get the tree and root of the xml file
                    tree = ET.parse(packageXML)
                    treeRoot = tree.getroot()

                    # Get all the tag elements
                    elements = []
                    namespaces = {"i": "https://wiki.freecad.org/Package_Metadata"}
                    elements = treeRoot.findall(
                        ".//i:content/i:preferencepack/i:tag", namespaces
                    )

                    # go throug all tags. If 'dark' in the element text, this is a dark theme
                    for element in elements:
                        if "dark" in element.text.lower():
                            IsDarkTheme = True
                            break

                except Exception:
                    if not os.path.isfile(packageXML):
                        if "dark" in currentStyleSheet.lower():
                            IsDarkTheme = True

    return IsDarkTheme


darkMode = DarkMode()


def _is_dark_theme_live() -> bool:
    try:
        return bool(DarkMode() is True)
    except Exception:
        return bool(darkMode)


def IsDarkInterface():
    try:
        if DarkMode() is True:
            return True
    except Exception:
        pass

    background = QColor(ReturnStyleItem("Background_Color", IgnoreOverlay=True))
    text = QColor(ReturnStyleItem("FontColor"))

    if background.isValid() and text.isValid():
        return background.lightness() < text.lightness()
    if background.isValid():
        return background.lightness() < 128

    fallbackBackground = QColor.fromRgba(
        preferences.GetUnsigned("GeneralBackgroundColor")
    )
    if fallbackBackground.isValid():
        return fallbackBackground.lightness() < 128

    return False


def ReturnStyleItem(ControlName, ShowCustomIcon=False, IgnoreOverlay=False):
    """
    Enter one of the names below:

    ControlName (string):
        "Background_Color" returns string,
        "Border_Color" returns string,
        "FontColor" returns string,
        "ApplicationButton_Background" returns string,
        "FontColor" returns string,
        "UpdateColor" returns string,
        "DevelopColor" returns string,
        "ScrollLeftButton_Tab returns QIcon",
        "ScrollRightButton_Tab" returns QIcon,
        "ScrollLeftButton_Category" returns QIcon,
        "ScrollRightButton_Category" returns QIcon,
        "OptionButton" returns QIcon,
        "PinButton_open" returns QIcon,
        "PinButton_closed" returns QIcon,
        "TitleBarButtons": returns list with icons,
    """
    # define a result holder and a dict for the StyleMapping file
    result = "none"

    # Get the current stylesheet for FreeCAD
    FreeCAD_preferences = App.ParamGet("User parameter:BaseApp/Preferences/MainWindow")
    currentStyleSheet = FreeCAD_preferences.GetString("StyleSheet")
    # if currentStyleSheet == "FreeCAD.qss":
    #     Theme = FreeCAD_preferences.GetString("Theme")
    #     if str("FreeCAD Light").lower() in Theme.lower():
    #         currentStyleSheet = "FreeCAD Light.qss"
    #     if str("FreeCAD Dark").lower() in Theme.lower():
    #         currentStyleSheet = "FreeCAD Dark.qss"
    
    # currentStyleSheet = "FreeCAD.qss"
    
    IsInList = False
    for key, _value in StyleMapping_default["Stylesheets"].items():
        if key == currentStyleSheet:
            IsInList = True
            break
    if IsInList is False:
        currentStyleSheet = "none"

    ListIcons = [
        "ScrollLeftButton_Tab",
        "ScrollRightButton_Tab",
        "ScrollLeftButton_Category",
        "ScrollRightButton_Category",
        "OptionButton",
        "PinButton_open",
        "PinButton_closed",
    ]

    isIcon = False
    for control in ListIcons:
        if control == ControlName:
            isIcon = True

    try:
        if ControlName == "TitleBarButtons":
            return StyleMapping["Stylesheets"][ControlName]
        if isIcon is True:
            result = None
            PixmapName = ""
            if Parameters_CommandTab.BETA_FUNCTIONS_ENABLED is True or ShowCustomIcon is True:
                PixmapName = StyleMapping["Stylesheets"][ControlName]
            else:
                PixmapName = ""
            if PixmapName == "" or PixmapName is None:
                PixmapName = StyleMapping_default["Stylesheets"][currentStyleSheet][
                    ControlName
                ]
                if PixmapName == "" or PixmapName is None:
                    PixmapName = StyleMapping_default["Stylesheets"][""][ControlName]
            if os.path.exists(PixmapName):
                pixmap = QPixmap(PixmapName)
            else:
                pixmap = QPixmap(os.path.join(pathIcons, PixmapName))
            result = QIcon()
            result.addPixmap(pixmap)
            return result
        if isIcon is False:
            result = ""

            if Parameters_CommandTab.CUSTOM_COLORS_ENABLED is True:
                if ControlName == "FontColor":
                    return ReturnFontColor()
                if ControlName == "UpdateColor":
                    return ReturnUpdateColor()
                if ControlName == "DevelopColor":
                    return ReturnDevelopColor()
                result = StyleMapping["Stylesheets"][ControlName]
            if (
                Parameters_CommandTab.BUTTON_BACKGROUND_ENABLED is False
                and Parameters_CommandTab.USE_FC_OVERLAY is True
                and ControlName == "Background_Color"
                and IgnoreOverlay is False
            ):
                result = "none"
            if result == "" or result is None:
                result = StyleMapping_default["Stylesheets"][currentStyleSheet][
                    ControlName
                ]
                if result == "" or result is None:
                    result = StyleMapping_default["Stylesheets"][""][ControlName]
            return result
    except Exception as e:
        print(e)
        return None


def ReturnStyleSheet(
    control,
    radius="2px",
    padding_left="0px",
    padding_top="0px",
    padding_right="0px",
    padding_bottom="0px",
    width="16px",
    HoverColor="",
):
    """
    Enter one of the names below:

    control (string):
        toolbutton,
        applicationbutton,
        DragIndicator,
    """
    StyleSheet = ""
    try:
        radiusValue = str(radius)
        if radiusValue.endswith("px") is False:
            radiusValue = radiusValue + "px"

        BorderColor = ReturnStyleItem("Border_Color")
        BackgroundColor = ReturnStyleItem("Background_Color")
        ApplicationButton = ReturnStyleItem("ApplicationButton_Background")
        if HoverColor == "":
            HoverColor = ReturnStyleItem("Background_Color_Hover")
        FontColor = ReturnStyleItem("FontColor")

        AppColor_1 = ApplicationButton
        if BackgroundColor is not None and BorderColor is not None:
            if Parameters_CommandTab.MODERN_COMMANDTAB_STYLE_ENABLED is True:
                isDarkTheme = IsDarkInterface()
                baseColor = QColor(BackgroundColor)
                if baseColor.isValid() is False:
                    baseColor = QColor("#eef2f7" if isDarkTheme is False else "#1f242c")
                hoverColor = QColor(HoverColor)
                if hoverColor.isValid() is False:
                    hoverColor = QColor("#dbeafe" if isDarkTheme is False else "#334155")
                borderColor = QColor(BorderColor)
                if borderColor.isValid() is False:
                    borderColor = QColor("#cbd5e1" if isDarkTheme is False else "#64748b")
                fontColor = QColor(FontColor)
                if fontColor.isValid() is False:
                    fontColor = QColor("#000000" if isDarkTheme is False else "#d7dde8")
                accentColor = QColor(ApplicationButton)
                if accentColor.isValid() is False:
                    accentColor = QColor("#2563eb")

                if isDarkTheme is True:
                    disabledTextColor = fontColor.darker(150).name()
                else:
                    disabledTextColor = fontColor.lighter(150).name()
                accentSoft = QColor(accentColor)
                accentSoft.setAlpha(42 if isDarkTheme is False else 58)
                accentStrong = QColor(accentColor)
                accentStrong.setAlpha(70 if isDarkTheme is False else 92)
                accentSoftCss = (
                    f"rgba({accentSoft.red()}, {accentSoft.green()}, "
                    + f"{accentSoft.blue()}, {accentSoft.alpha()})"
                )
                accentStrongCss = (
                    f"rgba({accentStrong.red()}, {accentStrong.green()}, "
                    + f"{accentStrong.blue()}, {accentStrong.alpha()})"
                )

                if control.lower() == "toolbutton":
                    StyleSheet = (
                        """QLayout {spacing: 0px}"""
                        + """QToolButton, QLabel, CommandTabToolButton, QLayout {
                        margin: 0px;
                        color: """
                        + fontColor.name()
                        + """;background: transparent;border: 1px solid transparent;
                        selection-background-color: transparent;
                        selection-color: """
                        + fontColor.name()
                        + """;border-radius: """
                        + radiusValue
                        + """;padding-left: """
                        + padding_left
                        + """;padding-top: """
                        + padding_top
                        + """;padding-bottom: """
                        + padding_bottom
                        + """;padding-right: """
                        + padding_right
                        + """;spacing: 0px;}"""
                        + """QToolButton::menu-arrow {
                        subcontrol-origin: padding;
                        subcontrol-position: center right;
                    }"""
                        + """QToolButton::menu-button {
                        margin: 0px;
                        padding: 0px;
                        width: """
                        + width
                        + """;
                        border-radius: """
                        + radiusValue
                        + """;border: 1px solid transparent;
                        subcontrol-origin: padding;
                        subcontrol-position: center right;
                    }"""
                        + """QToolButton:hover, QLabel:hover, CommandTabToolButton:hover {
                        background: """
                        + accentSoftCss
                        + """;border: 1px solid transparent;
                        color: """
                        + fontColor.name()
                        + """;}"""
                        + """QToolButton:pressed, QToolButton:checked {
                        background: """
                        + accentStrongCss
                        + """;border: 1px solid transparent;
                        color: """
                        + fontColor.name()
                        + """;}"""
                        + """QToolButton:disabled, QLabel:disabled {
                        background-color: transparent;
                        color: """
                        + disabledTextColor
                        + """;border: 1px solid transparent;}"""
                        + """QToolButton::menu-button:hover, QToolButton::menu-button:pressed {
                        background: """
                        + accentSoftCss
                        + """;border: 1px solid transparent;}"""
                        + """QToolButton:disabled::menu-button {
                        background-color: transparent;
                        border: 1px solid transparent;}"""
                    )
                    return StyleSheet

                if control.lower() == "applicationbutton":
                    baseAppColor = accentColor.name()
                    appHover = accentColor.lighter(110).name()
                    appPressed = accentColor.darker(108).name()
                    appBorder = accentColor.darker(115).name()
                    StyleSheet = (
                        """QToolButton {
                        border-radius: """
                        + radiusValue
                        + """;padding-right: """
                        + padding_right
                        + """;background-color: """
                        + baseAppColor
                        + """;color: """
                        + fontColor.name()
                        + """;border: 1px solid """
                        + appBorder
                        + """;}"""
                        + """QToolButton:hover {
                        background-color: """
                        + appHover
                        + """;border: 1px solid """
                        + accentColor.name()
                        + """;}"""
                        + """QToolButton:pressed {
                        background-color: """
                        + appPressed
                        + """;border: 1px solid """
                        + appBorder
                        + """;}"""
                    )
                    return StyleSheet

                if control.lower() == "dragindicator":
                    StyleSheet = (
                        """QLabel {
                        background-color: """
                        + accentSoftCss
                        + """;border-radius: """
                        + radiusValue
                        + """;}"""
                    )
                    return StyleSheet

            if control.lower() == "toolbutton":
                if Parameters_CommandTab.BORDER_TRANSPARANT is True:
                    BorderColor = BackgroundColor
                StyleSheet = (
                    """QLayout {spacing: 0px}"""
                    + """QToolButton, QLabel, CommandTabToolButton, QLayout {
                        margin: 0px;
                        padding: 0px;
                        color: """
                    + FontColor
                    + """;background: transparent;border: 1px solid transparent;padding-left: """
                    + padding_left
                    + """;padding-top: """
                    + padding_top
                    + """;padding-bottom: """
                    + padding_bottom
                    + """;padding-right: """
                    + padding_right
                    + """;spacing: 0px;}"""
                    + """QToolButton::menu-arrow {
                        subcontrol-origin: padding;
                        subcontrol-position: center right;
                    }"""
                    + """QToolButton::menu-button {
                        margin: 0px;
                        padding: 0px;
                        width: """
                    + width
                    + """;
                        border-radius: """
                    + radiusValue
                    + """;"""
                    + """padding: 0px;
                        subcontrol-origin: padding;
                        subcontrol-position: center right;
                    }"""
                    + """QToolButton:hover, QLabel:hover, CommandTabToolButton:hover {
                        margin: 0px 0px 0px 0px;
                        padding: 0px;"""
                    + """;background: """
                    + HoverColor
                    + """;padding-left: """
                    + padding_left
                    + """;padding-top: """
                    + padding_top
                    + """;padding-bottom: """
                    + padding_bottom
                    + """;padding-right: """
                    + padding_right
                    + """;border: 0.5px solid"""
                    + BorderColor
                    + """;}"""
                    + """QToolButton:disabled, QLabel:disabled {
                        background-color: transparent;
                        border: 1px solid transparent;
                    }"""
                )
                return StyleSheet
            if control.lower() == "applicationbutton":
                StyleSheet = (
                    """QToolButton {
                        border-radius : """
                    + radiusValue
                    + """;padding-right: """
                    + padding_right
                    + """;background-color: """
                    + AppColor_1
                    + """;border: 0.5px solid"""
                    + BorderColor
                    + """;}"""
                    + """QToolButton:hover { """
                    + """border: 2px solid"""
                    + BorderColor
                    + """;border-radius : """
                    + radiusValue
                    + """;}"""
                )
            if control.lower() == "dragindicator":
                StyleSheet = (
                    """QLabel {
                        background-color: """
                    + HoverColor
                    + """;border-radius: """
                    + radiusValue
                    + """;"""
                    + """;}"""
                )
            return StyleSheet
    except Exception as e:
        print(e)
        return StyleSheet


def GetIconBasedOnTag(ControlName=""):
    iconSet = {}
    iconName = ""
    IsDarkTheme = _is_dark_theme_live()

    # if it is a dark theme, get the white icons, else get the black icons
    if IsDarkTheme is True:
        iconSet = {
            "ScrollLeftButton_Tab": "backward_small_default_white.svg",
            "ScrollRightButton_Tab": "forward_small_default_white.svg",
            "ScrollLeftButton_Category": "backward_default_white.svg",
            "ScrollRightButton_Category": "forward_default_white.svg",
            "OptionButton": "more_default_white.svg",
            "PinButton_open": "pin-icon-open_white.svg",
            "PinButton_closed": "pin-icon-default_white.svg",
        }
    else:
        iconSet = {
            "ScrollLeftButton_Tab": "backward_small_default.svg",
            "ScrollRightButton_Tab": "forward_small_default.svg",
            "ScrollLeftButton_Category": "backward_default.svg",
            "ScrollRightButton_Category": "forward_default.svg",
            "OptionButton": "more_default.svg",
            "PinButton_open": "pin-icon-open.svg",
            "PinButton_closed": "pin-icon-default.svg",
        }

    # get the icon name for the requested control
    if ControlName != "":
        iconName = iconSet[ControlName]

    # return the icon name
    return iconName


def ReturnFontColor():
    fontColor = "#000000"
    IsDarkTheme = _is_dark_theme_live()

    if IsDarkTheme is True:
        fontColor = "#d7dde8"

    return fontColor


def ReturnUpdateColor():
    fontColor = "#CB7A00"
    IsDarkTheme = _is_dark_theme_live()

    if IsDarkTheme is True:
        fontColor = "#ffb340"

    return fontColor


def ReturnDevelopColor():
    fontColor = "#1B5E20"
    IsDarkTheme = _is_dark_theme_live()

    if IsDarkTheme is True:
        fontColor = "#538E1F"

    return fontColor


def ReturnTitleBarIcons():
    IconNames = [
        "close_default.svg",
        "maximize_default.svg",
        "restore_default.svg",
        "minimize_default.svg",
    ]
    IsDarkTheme = _is_dark_theme_live()

    if IsDarkTheme is True:
        IconNames = [
            "close_default_white.svg",
            "maximize_default_white.svg",
            "restore_default_white.svg",
            "minimize_default_white.svg",
        ]

    Icons = []
    for name in IconNames:
        pixMap = QPixmap(os.path.join(pathIcons, name))
        Icon = QIcon()
        Icon.addPixmap(pixMap)
        Icons.append(Icon)
    return Icons

def ReturnIcons_ThemeEditor():
    lightIcons = {
        "ScrollLeftButton_Tab": "backward_small_default.svg",
        "ScrollRightButton_Tab": "forward_small_default.svg",
        "ScrollLeftButton_Category": "backward_default.svg",
        "ScrollRightButton_Category": "forward_default.svg",
        "OptionButton": "more_default.svg",
        "PinButton_open": "pin-icon-open.svg",
        "PinButton_closed": "pin-icon-default.svg",
        "TitleBarButtons": ["maximize_default.svg", "restore_default.svg", "minimize_default.svg"]
    }
    
    darkIcons = {
        "ScrollLeftButton_Tab": "backward_small_default_white.svg",
        "ScrollRightButton_Tab": "forward_small_default_white.svg",
        "ScrollLeftButton_Category": "backward_default_white.svg",
        "ScrollRightButton_Category": "forward_default_white.svg",
        "OptionButton": "more_default_white.svg",
        "PinButton_open": "pin-icon-open_white.svg",
        "PinButton_closed": "pin-icon-default_white.svg",
        "TitleBarButtons": ["maximize_default_white.svg", "restore_default_white.svg", "minimize_default_white.svg"]
    }
    # if StandardFunctions.LightOrDark(color) == "dark":
    IsDarkTheme = _is_dark_theme_live()
    if IsDarkTheme is False:
        return lightIcons
    else:
        return darkIcons

# Used when custom colors are enabled
StyleMapping = {
    "Stylesheets": {
        "Background_Color": "",
        "Background_Color_Hover": Parameters_CommandTab.COLOR_BACKGROUND_HOVER,
        "Border_Color": Parameters_CommandTab.COLOR_BORDERS,
        "ApplicationButton_Background": Parameters_CommandTab.COLOR_APPLICATION_BUTTON_BACKGROUND,
        "FontColor": ReturnFontColor(),
        "UpdateColor": ReturnUpdateColor(),
        "DevelopColor": ReturnDevelopColor(),
        "ScrollLeftButton_Tab": Parameters_CommandTab.SCROLL_LEFT_BUTTON_TAB,
        "ScrollRightButton_Tab": Parameters_CommandTab.SCROLL_RIGHT_BUTTON_TAB,
        "ScrollLeftButton_Category": Parameters_CommandTab.SCROLL_LEFT_BUTTON_CATEGORY,
        "ScrollRightButton_Category": Parameters_CommandTab.SCROLL_RIGHT_BUTTON_CATEGORY,
        "OptionButton": Parameters_CommandTab.OPTION_BUTTON,
        "PinButton_open": Parameters_CommandTab.PIN_BUTTON_OPEN,
        "PinButton_closed": Parameters_CommandTab.PIN_BUTTON_CLOSED,
        "TitleBarButtons": ReturnTitleBarIcons(),
    }
}

StyleMapping_default = {
    "Stylesheets": {
        "": {
            "Background_Color": "none",
            "Background_Color_Hover": StandardFunctions.ColorConvertor(mw.palette().highlight().color().toTuple(), 1, True, False),
            "Border_Color": StandardFunctions.ColorConvertor(mw.palette().text().color().toTuple(), 1, True, False),
            "ApplicationButton_Background": StandardFunctions.ColorConvertor(mw.palette().highlight().color().toTuple(), 1, True, False),
            "FontColor": ReturnFontColor(),
            "UpdateColor": ReturnUpdateColor(),
            "DevelopColor": ReturnDevelopColor(),
            "ScrollLeftButton_Tab": "backward_small_default.svg",
            "ScrollRightButton_Tab": "forward_small_default.svg",
            "ScrollLeftButton_Category": "backward_default.svg",
            "ScrollRightButton_Category": "forward_default.svg",
            "OptionButton": "more_default.svg",
            "PinButton_open": "pin-icon-open.svg",
            "PinButton_closed": "pin-icon-default.svg",
            "TitleBarButtons": ReturnTitleBarIcons(),
        },
        "none": {
            "Background_Color": "none",
            "Background_Color_Hover": "#48a0f8",
            "Border_Color": ReturnFontColor(),
            "ApplicationButton_Background": "#48a0f8",
            "FontColor": ReturnFontColor(),
            "UpdateColor": ReturnUpdateColor(),
            "DevelopColor": ReturnDevelopColor(),
            "ScrollLeftButton_Tab": GetIconBasedOnTag("ScrollLeftButton_Tab"),
            "ScrollRightButton_Tab": GetIconBasedOnTag("ScrollRightButton_Tab"),
            "ScrollLeftButton_Category": GetIconBasedOnTag("ScrollLeftButton_Category"),
            "ScrollRightButton_Category": GetIconBasedOnTag(
                "ScrollRightButton_Category"
            ),
            "OptionButton": GetIconBasedOnTag("OptionButton"),
            "PinButton_open": GetIconBasedOnTag("PinButton_open"),
            "PinButton_closed": GetIconBasedOnTag("PinButton_closed"),
            "TitleBarButtons": ReturnTitleBarIcons(),
        },
        "FreeCAD.qss": {
            "Background_Color": "none",
            "Background_Color_Hover": "",
            "Border_Color": "",
            "ApplicationButton_Background": "",
            "FontColor": "",
            "UpdateColor": ReturnUpdateColor(),
            "DevelopColor": ReturnDevelopColor(),
            "ScrollLeftButton_Tab": ReturnIcons_ThemeEditor()["ScrollLeftButton_Tab"],
            "ScrollRightButton_Tab": ReturnIcons_ThemeEditor()["ScrollRightButton_Tab"],
            "ScrollLeftButton_Category": ReturnIcons_ThemeEditor()["ScrollLeftButton_Category"],
            "ScrollRightButton_Category": ReturnIcons_ThemeEditor()["ScrollRightButton_Category"],
            "OptionButton": ReturnIcons_ThemeEditor()["OptionButton"],
            "PinButton_open":  ReturnIcons_ThemeEditor()["PinButton_open"],
            "PinButton_closed": ReturnIcons_ThemeEditor()["PinButton_closed"],
            "TitleBarButtons": ReturnIcons_ThemeEditor()["TitleBarButtons"],
        },
        "FreeCAD Dark.qss": {
            "Background_Color": "none",
            "Background_Color_Hover": "#48a0f8",
            "Border_Color": "#ffffff",
            "ApplicationButton_Background": "#48a0f8",
            "FontColor": "#d7dde8",
            "UpdateColor": ReturnUpdateColor(),
            "DevelopColor": ReturnDevelopColor(),
            "ScrollLeftButton_Tab": "backward_small_default_white.svg",
            "ScrollRightButton_Tab": "forward_small_default_white.svg",
            "ScrollLeftButton_Category": "backward_default_white.svg",
            "ScrollRightButton_Category": "forward_default_white.svg",
            "OptionButton": "more_default_white.svg",
            "PinButton_open": "pin-icon-open_white.svg",
            "PinButton_closed": "pin-icon-default_white.svg",
            "TitleBarButtons": ReturnTitleBarIcons(),
        },
        "FreeCAD Light.qss": {
            "Background_Color": "none",
            "Background_Color_Hover": "#48a0f8",
            "Border_Color": "#646464",
            "ApplicationButton_Background": "#48a0f8",
            "FontColor": "#000000",
            "UpdateColor": ReturnUpdateColor(),
            "DevelopColor": ReturnDevelopColor(),
            "ScrollLeftButton_Tab": "backward_small_default.svg",
            "ScrollRightButton_Tab": "forward_small_default.svg",
            "ScrollLeftButton_Category": "backward_default.svg",
            "ScrollRightButton_Category": "forward_default.svg",
            "OptionButton": "more_default.svg",
            "PinButton_open": "pin-icon-open.svg",
            "PinButton_closed": "pin-icon-default.svg",
            "TitleBarButtons": ReturnTitleBarIcons(),
        },
        "OpenLight.qss": {
            "Background_Color": "none",
            "Background_Color_Hover": "#a5d8ff",
            "Border_Color": "#1c7ed6",
            "ApplicationButton_Background": "#a5d8ff",
            "FontColor": "#000000",
            "UpdateColor": ReturnUpdateColor(),
            "DevelopColor": ReturnDevelopColor(),
            "ScrollLeftButton_Tab": "backward_1.svg",
            "ScrollRightButton_Tab": "forward_1.svg",
            "ScrollLeftButton_Category": "backward_1.svg",
            "ScrollRightButton_Category": "forward_1.svg",
            "OptionButton": "more_1.svg",
            "PinButton_open": "pin-icon-open_1.svg",
            "PinButton_closed": "pin-icon-closed_1.svg",
            "TitleBarButtons": ReturnTitleBarIcons(),
        },
        "OpenDark.qss": {
            "Background_Color": "none",
            "Background_Color_Hover": "#1f364d",
            "Border_Color": "#264b69",
            "ApplicationButton_Background": "#1f364d",
            "FontColor": "#d7dde8",
            "UpdateColor": ReturnUpdateColor(),
            "DevelopColor": ReturnDevelopColor(),
            "ScrollLeftButton_Tab": "backward_small_default_white.svg",
            "ScrollRightButton_Tab": "forward_small_default_white.svg",
            "ScrollLeftButton_Category": "backward_default_white.svg",
            "ScrollRightButton_Category": "forward_default_white.svg",
            "OptionButton": "more_default_white.svg",
            "PinButton_open": "pin-icon-open_white.svg",
            "PinButton_closed": "pin-icon-default_white.svg",
            "TitleBarButtons": ReturnTitleBarIcons(),
        },
        "Behave-dark.qss": {
            "Background_Color": "#232932",
            "Background_Color_Hover": "#557bb6",
            "Border_Color": "#3a7400",
            "ApplicationButton_Background": "#557bb6",
            "FontColor": ReturnFontColor(),
            "UpdateColor": ReturnUpdateColor(),
            "DevelopColor": ReturnDevelopColor(),
            "ScrollLeftButton_Tab": "backward_small_default_white.svg",
            "ScrollRightButton_Tab": "forward_small_default_white.svg",
            "ScrollLeftButton_Category": "backward_default_white.svg",
            "ScrollRightButton_Category": "forward_default_white.svg",
            "OptionButton": "more_default_white.svg",
            "PinButton_open": "pin-icon-open_white.svg",
            "PinButton_closed": "pin-icon-default_white.svg",
            "TitleBarButtons": ReturnTitleBarIcons(),
        },
        "ProDark.qss": {
            "Background_Color": "#333333",
            "Background_Color_Hover": "#557bb6",
            "Border_Color": "#adc5ed",
            "ApplicationButton_Background": "#557bb6",
            "FontColor": ReturnFontColor(),
            "UpdateColor": ReturnUpdateColor(),
            "DevelopColor": ReturnDevelopColor(),
            "ScrollLeftButton_Tab": "backward_small_default_white.svg",
            "ScrollRightButton_Tab": "forward_small_default_white.svg",
            "ScrollLeftButton_Category": "backward_default_white.svg",
            "ScrollRightButton_Category": "forward_default_white.svg",
            "OptionButton": "more_default_white.svg",
            "PinButton_open": "pin-icon-open_white.svg",
            "PinButton_closed": "pin-icon-default_white.svg",
            "TitleBarButtons": ReturnTitleBarIcons(),
        },
        "Darker.qss": {
            "Background_Color": "#444444",
            "Background_Color_Hover": "#4aa5ff",
            "Border_Color": "#696968",
            "ApplicationButton_Background": "#4aa5ff",
            "FontColor": ReturnFontColor(),
            "ScrollLeftButton_Tab": "backward_small_default_white.svg",
            "ScrollRightButton_Tab": "forward_small_default_white.svg",
            "ScrollLeftButton_Category": "backward_default_white.svg",
            "ScrollRightButton_Category": "forward_default_white.svg",
            "OptionButton": "more_default_white.svg",
            "PinButton_open": "pin-icon-open_white.svg",
            "PinButton_closed": "pin-icon-default_white.svg",
            "TitleBarButtons": ReturnTitleBarIcons(),
        },
        "Light-modern.qss": {
            "Background_Color": "#f0f0f0",
            "Background_Color_Hover": "#4aa5ff",
            "Border_Color": "#646464",
            "ApplicationButton_Background": "#4aa5ff",
            "FontColor": ReturnFontColor(),
            "UpdateColor": ReturnUpdateColor(),
            "DevelopColor": ReturnDevelopColor(),
            "ScrollLeftButton_Tab": "backward_small_default.svg",
            "ScrollRightButton_Tab": "forward_small_default.svg",
            "ScrollLeftButton_Category": "backward_default.svg",
            "ScrollRightButton_Category": "forward_default.svg",
            "OptionButton": "more_default.svg",
            "PinButton_open": "pin-icon-open.svg",
            "PinButton_closed": "pin-icon-default.svg",
        },
        "Dark-modern.qss": {
            "Background_Color": "#2b2b2b",
            "Background_Color_Hover": "#4aa5ff",
            "Border_Color": "#ffffff",
            "ApplicationButton_Background": "#4aa5ff",
            "FontColor": ReturnFontColor(),
            "UpdateColor": ReturnUpdateColor(),
            "DevelopColor": ReturnDevelopColor(),
            "ScrollLeftButton_Tab": "backward_small_default_white.svg",
            "ScrollRightButton_Tab": "forward_small_default_white.svg",
            "ScrollLeftButton_Category": "backward_default_white.svg",
            "ScrollRightButton_Category": "forward_default_white.svg",
            "OptionButton": "more_default_white.svg",
            "PinButton_open": "pin-icon-open_white.svg",
            "PinButton_closed": "pin-icon-default_white.svg",
            "TitleBarButtons": ReturnTitleBarIcons(),
        },
        "Dark-contrast.qss": {
            "Background_Color": "#444444",
            "Background_Color_Hover": "#4aa5ff",
            "Border_Color": "#787878",
            "ApplicationButton_Background": "#4aa5ff",
            "FontColor": ReturnFontColor(),
            "UpdateColor": ReturnUpdateColor(),
            "DevelopColor": ReturnDevelopColor(),
            "ScrollLeftButton_Tab": "backward_small_default_white.svg",
            "ScrollRightButton_Tab": "forward_small_default_white.svg",
            "ScrollLeftButton_Category": "backward_default_white.svg",
            "ScrollRightButton_Category": "forward_default_white.svg",
            "OptionButton": "more_default_white.svg",
            "PinButton_open": "pin-icon-open_white.svg",
            "PinButton_closed": "pin-icon-default_white.svg",
            "TitleBarButtons": ReturnTitleBarIcons(),
        },
    }
}
