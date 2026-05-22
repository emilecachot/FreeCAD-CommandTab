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

import math
import os
import shutil
from collections import deque
from xml.etree.ElementTree import Element

import FreeCAD as App
import FreeCADGui as Gui

import Parameters_CommandTab
from freecad_commandtab.native.command_overrides import (
    CUSTOM_COMMAND_PIXMAP_FILES,
    SPECIAL_COMMAND_INFO_UPDATES,
    SPECIAL_PIXMAPS,
)

# Define the translation
translate = App.Qt.translate

mw = Gui.getMainWindow()

LOCAL_ICON_DIR = os.path.join(os.path.dirname(__file__), "Resources", "icons")
_EXTERNAL_ICON_THEME_ATTEMPTED = False
_EXTERNAL_ICON_THEME_REGISTERED = False
_EXTERNAL_ICON_THEME_NAME = "Flat"
_BUNDLED_COMMANDTAB_ICON_THEME_NAME = "FreeCAD-CommandTab-Modern"
_BUNDLED_ICON_THEME_FILE_EXTENSIONS = [".svg", ".png", ".xpm"]
_COMMANDTAB_ICON_CROP_RENDER_SIZE_DEFAULTS = [16, 20, 24, 32, 48, 64]
_COMMANDTAB_ICON_CROP_PADDING = 1
_CROPPED_COMMANDTAB_ICON_CACHE = {}


def commandtabUsesModernIconTheme() -> bool:
    return False


def ReturnLocalCommandTabIcon(iconName: str) -> str:
    return os.path.join(LOCAL_ICON_DIR, iconName)


def bundledCommandTabIconThemeSourcePath() -> str:
    return os.path.join(
        os.path.dirname(__file__), "Export", "IconThemes", _BUNDLED_COMMANDTAB_ICON_THEME_NAME
    )


def bundledCommandTabIconThemeInstallRoot() -> str:
    return os.path.join(App.getUserAppDataDir(), "Gui", "Icons")


def bundledCommandTabIconThemeInstallPath() -> str:
    return os.path.join(
        bundledCommandTabIconThemeInstallRoot(), _BUNDLED_COMMANDTAB_ICON_THEME_NAME
    )


def ensureBundledCommandTabIconThemeInstalled() -> bool:
    sourcePath = bundledCommandTabIconThemeSourcePath()
    targetPath = bundledCommandTabIconThemeInstallPath()

    if os.path.isdir(sourcePath) is False:
        return False

    os.makedirs(bundledCommandTabIconThemeInstallRoot(), exist_ok=True)

    sourceIndex = os.path.join(sourcePath, "index.theme")
    targetIndex = os.path.join(targetPath, "index.theme")
    shouldCopy = os.path.isdir(targetPath) is False
    if shouldCopy is False and os.path.exists(sourceIndex) and os.path.exists(targetIndex):
        shouldCopy = os.path.getmtime(sourceIndex) > os.path.getmtime(targetIndex)

    if shouldCopy is True:
        if os.path.isdir(targetPath):
            shutil.rmtree(targetPath)
        shutil.copytree(sourcePath, targetPath)
    return os.path.isdir(targetPath)


def enableBundledCommandTabIconTheme() -> bool:
    try:
        from PySide.QtGui import QIcon
    except Exception:
        return False

    if ensureBundledCommandTabIconThemeInstalled() is False:
        return False

    searchPaths = list(QIcon.themeSearchPaths())
    for candidate in [
        bundledCommandTabIconThemeInstallRoot(),
        os.path.join(os.path.dirname(__file__), "Export", "IconThemes"),
    ]:
        if candidate not in searchPaths and os.path.isdir(candidate):
            searchPaths.append(candidate)

    QIcon.setThemeSearchPaths(searchPaths)
    QIcon.setThemeName(_BUNDLED_COMMANDTAB_ICON_THEME_NAME)
    return True


def configureBundledCommandTabIconThemePreferences() -> bool:
    if ensureBundledCommandTabIconThemeInstalled() is False:
        return False

    themePreferences = App.ParamGet("User parameter:BaseApp/Preferences/Bitmaps/Theme")
    themePreferences.SetString("SearchPath", bundledCommandTabIconThemeInstallRoot())
    themePreferences.SetString("Name", _BUNDLED_COMMANDTAB_ICON_THEME_NAME)
    themePreferences.SetBool("UseIconTheme", True)
    themePreferences.SetBool("ThemeSearchPaths", True)
    App.saveParameter()
    return True


def activateBundledCommandTabIconTheme() -> bool:
    if Parameters_CommandTab.MODERN_COMMANDTAB_STYLE_ENABLED is False:
        return False
    if commandtabUsesModernIconTheme() is False:
        return False

    enabled = enableBundledCommandTabIconTheme()
    if enabled is False:
        return False

    try:
        configureBundledCommandTabIconThemePreferences()
    except Exception:
        pass
    return True


def _bundledCommandTabIconThemeDirectories() -> list:
    directories = []
    for root in [
        bundledCommandTabIconThemeInstallPath(),
        bundledCommandTabIconThemeSourcePath(),
    ]:
        scalableDir = os.path.join(root, "scalable")
        if scalableDir not in directories and os.path.isdir(scalableDir):
            directories.append(scalableDir)
    return directories


def resolveBundledCommandTabThemeIcon(iconName: str):
    from PySide.QtGui import QIcon

    if Parameters_CommandTab.MODERN_COMMANDTAB_STYLE_ENABLED is False:
        return QIcon()
    if commandtabUsesModernIconTheme() is False:
        return QIcon()

    if enableBundledCommandTabIconTheme() is False:
        return QIcon()

    iconName = os.path.basename(normalizeCommandTabCommandName(iconName))
    if iconName == "":
        return QIcon()

    stem = os.path.splitext(iconName)[0]
    themeCandidates = []
    if stem != "":
        themeCandidates.append(stem)
    if iconName not in themeCandidates:
        themeCandidates.append(iconName)

    for candidate in themeCandidates:
        try:
            icon = QIcon.fromTheme(candidate)
            if icon.isNull() is False:
                return icon
        except Exception:
            pass

    fileCandidates = []
    if os.path.splitext(iconName)[1] != "":
        fileCandidates.append(iconName)
    for candidate in [stem, iconName]:
        if candidate in ["", None]:
            continue
        for suffix in _BUNDLED_ICON_THEME_FILE_EXTENSIONS:
            fileName = f"{candidate}{suffix}"
            if fileName not in fileCandidates:
                fileCandidates.append(fileName)

    for directory in _bundledCommandTabIconThemeDirectories():
        for fileName in fileCandidates:
            iconPath = os.path.join(directory, fileName)
            if os.path.exists(iconPath):
                icon = QIcon(iconPath)
                if icon.isNull() is False:
                    return icon

    return QIcon()


def _externalIconThemeBasePaths() -> list:
    userAppDir = App.getUserAppDataDir()
    siblingAddonDir = os.path.abspath(
        os.path.join(os.path.dirname(__file__), "..", "IconThemes")
    )
    candidateRoots = [
        os.path.join(userAppDir, "Mod", "IconThemes"),
        siblingAddonDir,
    ]

    normalizedRoots = []
    for root in candidateRoots:
        if root not in normalizedRoots:
            normalizedRoots.append(root)
    return normalizedRoots


def _externalIconThemeDirectories() -> list:
    directories = []
    for root in _externalIconThemeBasePaths():
        candidates = [
            os.path.join(root, "FreeCAD-Flat-Icons-main", "icons", "Flat", "scalable"),
            os.path.join(root, "icons", "Flat", "scalable"),
            os.path.join(App.getUserAppDataDir(), "Gui", "Icons", "Flat", "scalable"),
        ]
        for candidate in candidates:
            if candidate not in directories:
                directories.append(candidate)
    return directories


def _externalIconThemeRccCandidates() -> list:
    candidates = []
    for root in _externalIconThemeBasePaths():
        for fileName in ["FlatDark.rcc", "Flat_Dark.rcc"]:
            candidate = os.path.join(root, fileName)
            if candidate not in candidates:
                candidates.append(candidate)
            candidate = os.path.join(root, "FreeCAD-Flat-Icons-main", fileName)
            if candidate not in candidates:
                candidates.append(candidate)
    return candidates


def ensureExternalCommandTabIconTheme() -> bool:
    global _EXTERNAL_ICON_THEME_ATTEMPTED
    global _EXTERNAL_ICON_THEME_REGISTERED

    if _EXTERNAL_ICON_THEME_ATTEMPTED is True:
        return _EXTERNAL_ICON_THEME_REGISTERED

    _EXTERNAL_ICON_THEME_ATTEMPTED = True

    try:
        from PySide.QtCore import QFile, QResource
    except Exception:
        return False

    if QFile.exists(f":/icons/{_EXTERNAL_ICON_THEME_NAME}/index.theme"):
        _EXTERNAL_ICON_THEME_REGISTERED = True
        return True

    for candidate in _externalIconThemeRccCandidates():
        if os.path.exists(candidate) is False:
            continue
        try:
            if QResource.registerResource(candidate):
                _EXTERNAL_ICON_THEME_REGISTERED = True
                return True
        except Exception:
            pass

    return False


def resolveExternalCommandTabThemeIcon(iconName: str):
    from PySide.QtGui import QIcon

    if Parameters_CommandTab.MODERN_COMMANDTAB_STYLE_ENABLED is False:
        return QIcon()

    if Parameters_CommandTab.USE_EXTERNAL_ICON_THEME_FALLBACK is False:
        return QIcon()

    iconName = normalizeCommandTabCommandName(iconName)
    if iconName == "":
        return QIcon()

    if iconName.endswith(".svg") is False:
        iconName = f"{iconName}.svg"

    ensureExternalCommandTabIconTheme()

    try:
        from PySide.QtCore import QFile
    except Exception:
        QFile = None

    resourceCandidates = [
        f":/icons/{_EXTERNAL_ICON_THEME_NAME}/scalable/{iconName}",
        f":/{iconName}",
    ]
    for resourcePath in resourceCandidates:
        try:
            if QFile is not None and QFile.exists(resourcePath):
                icon = QIcon(resourcePath)
                if icon.isNull() is False:
                    return icon
        except Exception:
            pass

    for themeDir in _externalIconThemeDirectories():
        themePath = os.path.join(themeDir, iconName)
        if os.path.exists(themePath):
            icon = QIcon(themePath)
            if icon.isNull() is False:
                return icon

    return QIcon()


def _commandtabIconCropCacheIdentity(icon=None, cacheKey: str = "") -> str:
    if cacheKey not in ["", None]:
        return str(cacheKey)

    if icon is None:
        return ""

    try:
        return str(icon.cacheKey())
    except Exception:
        return ""


def _commandtabIconCropRenderSizes() -> list:
    sizes = list(_COMMANDTAB_ICON_CROP_RENDER_SIZE_DEFAULTS)
    for candidate in [
        getattr(Parameters_CommandTab, "ICON_SIZE_SMALL", 0),
        getattr(Parameters_CommandTab, "ICON_SIZE_MEDIUM", 0),
        getattr(Parameters_CommandTab, "ICON_SIZE_LARGE", 0),
        getattr(Parameters_CommandTab, "APP_ICON_SIZE", 0),
        getattr(Parameters_CommandTab, "QUICK_ICON_SIZE", 0),
        getattr(Parameters_CommandTab, "RIGHT_ICON_SIZE", 0),
        getattr(Parameters_CommandTab, "TABBAR_SIZE", 0),
    ]:
        try:
            candidate = int(candidate)
        except Exception:
            continue
        if candidate > 0 and candidate not in sizes:
            sizes.append(candidate)
    sizes.sort()
    return sizes


def _findCommandTabOpaquePixmapBounds(pixmap):
    from PySide.QtCore import QRect

    if pixmap is None or pixmap.isNull():
        return QRect()

    image = pixmap.toImage()
    width = image.width()
    height = image.height()
    if width <= 0 or height <= 0:
        return QRect()

    fullBounds = QRect(0, 0, width, height)
    if image.hasAlphaChannel() is False:
        return fullBounds

    left = width
    top = height
    right = -1
    bottom = -1

    for y in range(height):
        for x in range(width):
            if ((image.pixel(x, y) >> 24) & 0xFF) == 0:
                continue

            if x < left:
                left = x
            if x > right:
                right = x
            if y < top:
                top = y
            if y > bottom:
                bottom = y

    if right < left or bottom < top:
        return QRect()

    bounds = QRect(left, top, right - left + 1, bottom - top + 1)
    if _COMMANDTAB_ICON_CROP_PADDING > 0:
        bounds = bounds.adjusted(
            -_COMMANDTAB_ICON_CROP_PADDING,
            -_COMMANDTAB_ICON_CROP_PADDING,
            _COMMANDTAB_ICON_CROP_PADDING,
            _COMMANDTAB_ICON_CROP_PADDING,
        ).intersected(fullBounds)
    return bounds


def _cropCommandTabPixmapTransparentMargins(pixmap):
    if pixmap is None or pixmap.isNull():
        return pixmap

    bounds = _findCommandTabOpaquePixmapBounds(pixmap)
    if bounds.isNull() or bounds.width() <= 0 or bounds.height() <= 0:
        return pixmap

    if (
        bounds.x() == 0
        and bounds.y() == 0
        and bounds.width() == pixmap.width()
        and bounds.height() == pixmap.height()
    ):
        return pixmap

    croppedPixmap = pixmap.copy(bounds)
    try:
        croppedPixmap.setDevicePixelRatio(pixmap.devicePixelRatio())
    except Exception:
        pass
    return croppedPixmap


def _colorsAreClose(colorA, colorB, tolerance: int = 18) -> bool:
    if colorA.alpha() == 0 and colorB.alpha() == 0:
        return True
    return (
        abs(colorA.red() - colorB.red()) <= tolerance
        and abs(colorA.green() - colorB.green()) <= tolerance
        and abs(colorA.blue() - colorB.blue()) <= tolerance
        and abs(colorA.alpha() - colorB.alpha()) <= max(24, tolerance * 2)
    )


def _stripCommandTabPixmapFlatBackground(pixmap):
    from PySide.QtGui import QColor, QImage, QPixmap

    if pixmap is None or pixmap.isNull():
        return pixmap

    image = pixmap.toImage().convertToFormat(QImage.Format_ARGB32)
    width = image.width()
    height = image.height()
    if width < 4 or height < 4:
        return pixmap

    corner_points = [
        (0, 0),
        (width - 1, 0),
        (0, height - 1),
        (width - 1, height - 1),
    ]
    corner_colors = [image.pixelColor(x, y) for x, y in corner_points]
    opaque_corners = [color for color in corner_colors if color.alpha() >= 220]
    if len(opaque_corners) < 3:
        return pixmap

    reference = opaque_corners[0]
    similar_corners = [color for color in opaque_corners if _colorsAreClose(color, reference)]
    if len(similar_corners) < 3:
        return pixmap

    visited = [[False] * width for _ in range(height)]
    queue = deque()

    def enqueue_if_match(x: int, y: int) -> None:
        if x < 0 or y < 0 or x >= width or y >= height or visited[y][x]:
            return
        color = image.pixelColor(x, y)
        if color.alpha() < 200 or not _colorsAreClose(color, reference):
            return
        visited[y][x] = True
        queue.append((x, y))

    for x in range(width):
        enqueue_if_match(x, 0)
        enqueue_if_match(x, height - 1)
    for y in range(height):
        enqueue_if_match(0, y)
        enqueue_if_match(width - 1, y)

    cleared_pixels = 0
    transparent = QColor(0, 0, 0, 0)
    while queue:
        x, y = queue.popleft()
        image.setPixelColor(x, y, transparent)
        cleared_pixels += 1
        enqueue_if_match(x - 1, y)
        enqueue_if_match(x + 1, y)
        enqueue_if_match(x, y - 1)
        enqueue_if_match(x, y + 1)

    if cleared_pixels == 0:
        return pixmap

    stripped = QPixmap.fromImage(image)
    try:
        stripped.setDevicePixelRatio(pixmap.devicePixelRatio())
    except Exception:
        pass
    return _cropCommandTabPixmapTransparentMargins(stripped)


def cropCommandTabIconTransparentMargins(icon, cacheKey: str = ""):
    from PySide.QtCore import QSize
    from PySide.QtGui import QIcon

    if icon is None:
        return QIcon()

    if icon.isNull():
        return icon

    identity = _commandtabIconCropCacheIdentity(icon, cacheKey)
    if identity != "" and identity in _CROPPED_COMMANDTAB_ICON_CACHE:
        return _CROPPED_COMMANDTAB_ICON_CACHE[identity]

    croppedIcon = QIcon()
    addedPixmap = False
    addedVariants = set()

    for mode in [
        QIcon.Mode.Normal,
        QIcon.Mode.Disabled,
        QIcon.Mode.Active,
        QIcon.Mode.Selected,
    ]:
        for state in [QIcon.State.Off, QIcon.State.On]:
            for edge in _commandtabIconCropRenderSizes():
                pixmap = icon.pixmap(QSize(edge, edge), mode, state)
                if pixmap is None or pixmap.isNull():
                    continue

                croppedPixmap = _stripCommandTabPixmapFlatBackground(
                    _cropCommandTabPixmapTransparentMargins(pixmap)
                )
                if croppedPixmap is None or croppedPixmap.isNull():
                    continue

                try:
                    variantKey = (mode, state, int(croppedPixmap.cacheKey()))
                except Exception:
                    variantKey = (
                        mode,
                        state,
                        croppedPixmap.width(),
                        croppedPixmap.height(),
                    )
                if variantKey in addedVariants:
                    continue

                croppedIcon.addPixmap(croppedPixmap, mode, state)
                addedVariants.add(variantKey)
                addedPixmap = True

    if addedPixmap is False:
        croppedIcon = icon

    if identity != "":
        _CROPPED_COMMANDTAB_ICON_CACHE[identity] = croppedIcon
    return croppedIcon


CUSTOM_COMMAND_PIXMAPS = {
    command_name: ReturnLocalCommandTabIcon(icon_name)
    for command_name, icon_name in CUSTOM_COMMAND_PIXMAP_FILES.items()
}


def normalizeCommandTabCommandName(CommandName) -> str:
    if CommandName in [None, ""]:
        return ""
    try:
        return str(CommandName).strip()
    except Exception:
        return ""


def commandtabCommandIconCandidates(CommandName: str) -> list:
    commandName = normalizeCommandTabCommandName(CommandName)
    if commandName == "":
        return []

    candidates = [commandName]

    if commandName.endswith("_ddb"):
        candidates.append(commandName[:-4])

    if ", " in commandName:
        parentCommand = commandName.split(", ")[0].strip()
        if parentCommand != "":
            candidates.append(parentCommand)
            if parentCommand.endswith("_ddb"):
                candidates.append(parentCommand[:-4])

    orderedCandidates = []
    for candidate in candidates:
        if candidate not in orderedCandidates and candidate not in [None, ""]:
            orderedCandidates.append(candidate)
    return orderedCandidates


def resolveCommandTabIconCommand(CommandName: str) -> str:
    for candidate in commandtabCommandIconCandidates(CommandName):
        if candidate in CUSTOM_COMMAND_PIXMAPS:
            return candidate
    candidates = commandtabCommandIconCandidates(CommandName)
    if len(candidates) > 0:
        return candidates[0]
    return ""


def resolveCommandTabPreferredPixmap(CommandName: str, pixmap: str = "") -> str:
    if (
        Parameters_CommandTab.MODERN_COMMANDTAB_STYLE_ENABLED is True
        and commandtabUsesModernIconTheme() is True
        and resolveCommandTabIconCommand(CommandName) in CUSTOM_COMMAND_PIXMAPS
    ):
        return CUSTOM_COMMAND_PIXMAPS[resolveCommandTabIconCommand(CommandName)]
    return pixmap


def _resolveBundledCommandTabThemeIconPath(iconName: str) -> str:
    iconName = os.path.basename(normalizeCommandTabCommandName(iconName))
    if iconName == "":
        return ""

    stem = os.path.splitext(iconName)[0]
    fileCandidates = []
    if os.path.splitext(iconName)[1] != "":
        fileCandidates.append(iconName)
    for candidate in [stem, iconName]:
        if candidate in ["", None]:
            continue
        for suffix in _BUNDLED_ICON_THEME_FILE_EXTENSIONS:
            fileName = f"{candidate}{suffix}"
            if fileName not in fileCandidates:
                fileCandidates.append(fileName)

    for directory in _bundledCommandTabIconThemeDirectories():
        for fileName in fileCandidates:
            iconPath = os.path.join(directory, fileName)
            if os.path.exists(iconPath):
                return iconPath
    return ""


def _resolveExternalCommandTabThemeIconPath(iconName: str) -> str:
    if Parameters_CommandTab.USE_EXTERNAL_ICON_THEME_FALLBACK is False:
        return ""

    iconName = normalizeCommandTabCommandName(iconName)
    if iconName == "":
        return ""

    if iconName.endswith(".svg") is False:
        iconName = f"{iconName}.svg"

    for themeDir in _externalIconThemeDirectories():
        themePath = os.path.join(themeDir, iconName)
        if os.path.exists(themePath):
            return themePath
    return ""


def resolveCommandTabNativeIconPath(CommandName: str, pixmap: str = "") -> str:
    originalCommandName = normalizeCommandTabCommandName(CommandName)
    requestedPixmap = pixmap
    CommandName = resolveCommandTabIconCommand(CommandName)
    preferredPixmap = resolveCommandTabPreferredPixmap(CommandName, pixmap)

    if preferredPixmap not in ["", None]:
        if os.path.exists(preferredPixmap):
            return preferredPixmap

        localPixmap = os.path.join(LOCAL_ICON_DIR, preferredPixmap)
        if os.path.exists(localPixmap):
            return localPixmap

        bundledPath = _resolveBundledCommandTabThemeIconPath(os.path.basename(preferredPixmap))
        if bundledPath != "":
            return bundledPath

        externalPath = _resolveExternalCommandTabThemeIconPath(os.path.basename(preferredPixmap))
        if externalPath != "":
            return externalPath

    for candidate in commandtabCommandIconCandidates(originalCommandName):
        bundledPath = _resolveBundledCommandTabThemeIconPath(candidate)
        if bundledPath != "":
            return bundledPath

        externalPath = _resolveExternalCommandTabThemeIconPath(candidate)
        if externalPath != "":
            return externalPath

    try:
        Command = Gui.Command.get(CommandName)
        CommandInfo = Command.getInfo()
        commandPixmap = str(CommandInfo.get("pixmap") or requestedPixmap or "")
        if commandPixmap != "":
            if os.path.exists(commandPixmap):
                return commandPixmap
            localPixmap = os.path.join(LOCAL_ICON_DIR, commandPixmap)
            if os.path.exists(localPixmap):
                return localPixmap
    except Exception:
        pass

    return ""


def resolveCommandTabActionCommandName(action, fallbackCommandName: str = "") -> str:
    fallbackCommandName = normalizeCommandTabCommandName(fallbackCommandName)
    if fallbackCommandName != "":
        return fallbackCommandName

    if action is None:
        return ""

    try:
        actionData = normalizeCommandTabCommandName(action.data())
        if actionData != "":
            return actionData
    except Exception:
        pass

    try:
        objectName = normalizeCommandTabCommandName(action.objectName())
        if objectName != "":
            return objectName
    except Exception:
        pass

    return ""


def applyPreferredIconToAction(action, fallbackCommandName: str = "", pixmap: str = ""):
    from PySide.QtGui import QIcon

    if action is None:
        return QIcon()

    commandName = resolveCommandTabActionCommandName(action, fallbackCommandName)
    icon = returnQiCons_Commands(commandName, pixmap)
    if icon is None or icon.isNull():
        try:
            return cropCommandTabIconTransparentMargins(
                action.icon(), f"action:{commandName}"
            )
        except Exception:
            return QIcon()

    try:
        if normalizeCommandTabCommandName(action.data()) == "" and commandName != "":
            action.setData(commandName)
    except Exception:
        pass

    try:
        icon = cropCommandTabIconTransparentMargins(icon, f"action:{commandName}")
        action.setIcon(icon)
    except Exception:
        pass
    return icon


def Mbox(
    text,
    title="",
    style=0,
    IconType="Information",
    default="",
    stringList="[,]",
):
    """
    Message Styles:\n
    0 : OK                          (text, title, style)\n
    1 : Yes | No                    (text, title, style)\n
    2 : Ok | Cancel                 (text, title, style)\n
    20 : Inputbox                   (text, title, style, default)\n
    21 : Inputbox with dropdown     (text, title, style, default, stringlist)\n
    Icontype:                       string: NoIcon, Question, Warning, Critical. Default Information\n
    30 : OK (Non blocking)          (text, title, style)\n
    """
    from PySide.QtCore import Qt
    from PySide.QtWidgets import QInputDialog, QMessageBox

    Icon = QMessageBox.Icon.Information
    if IconType == "NoIcon":
        Icon = QMessageBox.Icon.NoIcon
    if IconType == "Question":
        Icon = QMessageBox.Icon.Question
    if IconType == "Warning":
        Icon = QMessageBox.Icon.Warning
    if IconType == "Critical":
        Icon = QMessageBox.Icon.Critical

    if style == 0:
        # Set the messagebox
        msgBox = QMessageBox()
        msgBox.setIcon(Icon)
        msgBox.setText(text)
        msgBox.setWindowTitle(title)

        reply = msgBox.exec()
        if reply == QMessageBox.StandardButton.Ok:
            return "ok"
    if style == 1:
        # Set the messagebox
        msgBox = QMessageBox()
        msgBox.setIcon(Icon)
        msgBox.setText(text)
        msgBox.setWindowTitle(title)
        # Set the buttons and default button
        msgBox.setStandardButtons(
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )
        msgBox.setDefaultButton(QMessageBox.StandardButton.No)

        reply = msgBox.exec_()
        if reply == QMessageBox.StandardButton.Yes:
            return "yes"
        if reply == QMessageBox.StandardButton.No:
            return "no"
    if style == 2:
        # Set the messagebox
        msgBox = QMessageBox()
        msgBox.setIcon(Icon)
        msgBox.setText(text)
        msgBox.setWindowTitle(title)
        # Set the buttons and default button
        msgBox.setStandardButtons(
            QMessageBox.StandardButton.Ok | QMessageBox.StandardButton.Cancel
        )
        msgBox.setDefaultButton(QMessageBox.StandardButton.Ok)

        reply = msgBox.exec_()
        if reply == QMessageBox.StandardButton.Ok:
            return "ok"
        if reply == QMessageBox.StandardButton.Cancel:
            return "cancel"
    if style == 20:
        Dialog = QInputDialog()
        reply = Dialog.getText(
            None,
            title,
            text,
            text=default,
        )
        if reply[1]:
            # user clicked OK
            replyText = reply[0]
        else:
            # user clicked Cancel
            replyText = reply[0]  # which will be "" if they clicked Cancel
        return str(replyText)
    if style == 21:
        Dialog = QInputDialog()
        reply = Dialog.getItem(
            None,
            title,
            text,
            stringList,
            0,
            True,
        )
        if reply[1]:
            # user clicked OK
            replyText = reply[0]
        else:
            # user clicked Cancel
            replyText = reply[0]  # which will be "" if they clicked Cancel
        return str(replyText)
    if style == 30:
        # Set the messagebox
        msgBox = QMessageBox(mw)
        msgBox.setIcon(Icon)
        msgBox.setText(text)
        msgBox.setWindowTitle(title)
        msgBox.setWindowModality(Qt.WindowModality.NonModal)
        msgBox.setWindowFlags(Qt.WindowType.WindowStaysOnTopHint | Qt.WindowType.Dialog)

        reply = msgBox.show()
        return


def RestartDialog(message="", includeIcons=False):
    """_summary_
        shows a restart dialog
    Returns:
        string: returns 'yes' if restart now is clicked.
        otherwise returns 'no'
    """
    from PySide.QtWidgets import QMessageBox

    # Save the preferences before restarting
    App.saveParameter()

    # Set the message
    if message == "":
        message = translate(
            "FreeCAD CommandTab",
            "You must restart FreeCAD for changes to take effect.",
        )

    # Set the messagebox
    msgBox = QMessageBox()
    msgBox.setIcon(QMessageBox.Icon.Warning)
    msgBox.setText(message)
    msgBox.setWindowTitle(translate("FreeCAD CommandTab", "FreeCAD CommandTab"))
    # Set the buttons and default button
    msgBox.setStandardButtons(
        QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
    )
    msgBox.setDefaultButton(QMessageBox.StandardButton.No)
    msgBox.button(QMessageBox.StandardButton.Yes).setText(
        translate("FreeCAD CommandTab", "Restart now")
    )
    msgBox.button(QMessageBox.StandardButton.No).setText(
        translate("FreeCAD CommandTab", "Restart later")
    )
    if includeIcons is True:
        msgBox.button(QMessageBox.StandardButton.No).setIcon(
            Gui.getIcon("edit_Cancel.svg")
        )
        msgBox.button(QMessageBox.StandardButton.Yes).setIcon(
            Gui.getIcon("edit_OK.svg")
        )

    reply = msgBox.exec_()
    if reply == QMessageBox.StandardButton.Yes:
        return "yes"
    if reply == QMessageBox.StandardButton.No:
        return "no"


def restart_freecad():
    from PySide import QtCore, QtWidgets

    """Shuts down and restarts FreeCAD"""

    args = QtWidgets.QApplication.arguments()[1:]
    if Gui.getMainWindow().close():
        QtCore.QProcess.startDetached(
            QtWidgets.QApplication.applicationFilePath(), args
        )

    return


def SaveDialog(files, OverWrite: bool = True):
    """
    files must be like:\n
    files = [\n
        ('All Files', '*.*'),\n
        ('Python Files', '*.py'),\n
        ('Text Document', '*.txt')\n
    ]\n
    \n
    OverWrite:\n
    If True, file will be overwritten\n
    If False, only the path+filename will be returned\n
    """
    import tkinter as tk
    from tkinter.filedialog import askopenfilename, asksaveasfile

    # Create the window
    root = tk.Tk()
    # Hide the window
    root.withdraw()

    if OverWrite is True:
        file = asksaveasfile(filetypes=files, defaultextension=files)
        if file is not None:
            return file.name
    if OverWrite is False:
        file = askopenfilename(filetypes=files, defaultextension=files)
        if file is not None:
            return file

def OpenDirectory(path):
    import os
    import platform
    import subprocess
    
    try:
        if os.path.exists(path) is False:
            return False
        
        if platform.system().lower() == "darwin":
                subprocess.run(['open', path])
        elif platform.system().lower() == "Windows":
            os.startfile(path)
        else:
            # Linux: try xdg-open, then sensible-browser as fallback 
            try: 
                subprocess.run(['xdg-open', path], check=True) 
            except Exception: 
                subprocess.run(['gio', 'open', path], check=False)          
        return True
    except Exception:
        return False

def GetLetterFromNumber(number: int, UCase: bool = True):
    """Number to Excel-style column name, e.g., 1 = A, 26 = Z, 27 = AA, 703 = AAA."""
    Letter = ""
    while number > 0:
        number, r = divmod(number - 1, 26)
        Letter = chr(r + ord("A")) + Letter
    return Letter


def GetNumberFromLetter(Letter):
    """Excel-style column name to number, e.g., A = 1, Z = 26, AA = 27, AAA = 703."""
    number = 0
    for c in Letter:
        number = number * 26 + 1 + ord(c) - ord("A")
    return number


def ColorConvertor(ColorRGB: [], Alpha: float = 1, Hex=False, KeepHexAlpha=True):
    """
    A single function to convert colors to rgba colors as a tuple of float from 0-1
    ColorRGB:   [255,255,255]
    Alpha:      0-1
    """
    def _clamp_unit(value: float) -> float:
        if value < 0:
            return 0.0
        if value > 1:
            return 1.0
        return float(value)

    red = _clamp_unit(float(ColorRGB[0]) / 255.0)
    green = _clamp_unit(float(ColorRGB[1]) / 255.0)
    blue = _clamp_unit(float(ColorRGB[2]) / 255.0)
    alpha = _clamp_unit(float(Alpha))

    if Hex is False:
        return (red, green, blue, alpha)

    red_hex = f"{int(round(red * 255)):02x}"
    green_hex = f"{int(round(green * 255)):02x}"
    blue_hex = f"{int(round(blue * 255)):02x}"

    if KeepHexAlpha is True:
        alpha_hex = f"{int(round(alpha * 255)):02x}"
        return f"#{red_hex}{green_hex}{blue_hex}{alpha_hex}"

    return f"#{red_hex}{green_hex}{blue_hex}"


def OpenFile(FileName: str):
    """
    Filename = full path with filename as string
    """
    import os
    import platform
    import subprocess

    try:
        if os.path.exists(FileName):
            if platform.system() == "Darwin":  # macOS
                subprocess.call(("open", FileName))
            elif platform.system() == "Windows":  # Windows
                os.startfile(FileName)
            else:  # linux variants
                print(FileName)
                try:
                    subprocess.check_output(["xdg-open", FileName.strip()])
                except subprocess.CalledProcessError:
                    Print(
                        f"An error occurred when opening {FileName}!\n"
                        + "This can happen when running FreeCAD as an AppImage.\n"
                        + "Please install FreeCAD directly.",
                        "Error",
                    )
        else:
            print(f"Error: {FileName} does not exist.")
    except Exception as e:
        raise e


def Print(Input: str, Type: str = ""):
    """_summary_

    Args:
        Input (str): Text to print.\n
        Type (str, optional): Type of message. (enter Warning, Error or Log). Defaults to "".
    """
    import FreeCAD as App

    if Type == "Warning":
        App.Console.PrintWarning(Input + "\n")
    elif Type == "Error":
        App.Console.PrintError(Input + "\n")
    elif Type == "Log":
        App.Console.PrintLog(Input + "\n")
    else:
        App.Console.PrintMessage(Input + "\n")


def LightOrDark(rgbColor=None):
    """_summary_
    reference: https://alienryderflex.com/hsp.html
    Args:
        rgbColor (list, optional): RGB color. Defaults to [0, 128, 255, 255].\n
        note: The alpha value is added for completeness, but us ignored in the equation.

    Returns:
        string: "light or dark"
    """
    if rgbColor is None:
        rgbColor = [0, 128, 255, 255]
    [r, g, b, a] = rgbColor
    hsp = math.sqrt(0.299 * (r * r) + 0.587 * (g * g) + 0.114 * (b * b))
    if hsp > 127.5:
        return "light"
    else:
        return "dark"


def GetFileDialog(Filter="", parent=None, DefaultPath="", SaveAs: bool = True) -> str:
    """
    Set filter like:
    "Images (*.png *.xpm .jpg);;Text files (.txt);;XML files (*.xml)"
    SaveAs:\n
        If True,  as SaveAs dialog will open and the file will be overwritten\n
        If False, an OpenFile dialog will be open and the file will be opened.\n
    """
    from PySide.QtWidgets import QFileDialog

    file = ""
    if SaveAs is False:
        file = QFileDialog.getOpenFileName(
            parent=parent, caption="Select a file", dir=DefaultPath, filter=Filter
        )[0]
    if SaveAs is True:
        file = QFileDialog.getSaveFileName(
            parent=parent, caption="Select a file", dir=DefaultPath, filter=Filter
        )[0]
    return file


def GetFolder(parent=None, DefaultPath="") -> str:
    from PySide.QtWidgets import QFileDialog

    Directory = ""
    Directory = QFileDialog.getExistingDirectory(
        parent=parent, caption="Select Folder", dir=DefaultPath
    )

    return Directory


def getRepoAdress(base_path):
    import os
    import pathlib

    try:
        if base_path == "":
            base_path = os.path.dirname(__file__)

        git_dir = pathlib.Path(base_path) / ".git"
        with (git_dir / "FETCH_HEAD").open("r") as head:
            ref = head.readline().split(" ")[-1].strip()

        return ref
    except Exception:
        return ""


def CreateToolbar(Name: str, WorkBenchName: str = "Global", ButtonList=None):
    if ButtonList is None:
        ButtonList = []
    # Define the name for the ToolbarGroup in the FreeCAD Parameters
    ToolbarGroupName = WorkBenchName
    # Define the name for the toolbar
    ToolBarName = Name
    # define the parameter path for the toolbar
    WorkbenchToolBarsParamPath = (
        "User parameter:BaseApp/Workbench/" + ToolbarGroupName + "/Toolbar/"
    )

    # check if there is already a toolbar with the same name
    CustomToolbars: list = App.ParamGet(
        "User parameter:BaseApp/Workbench/Global/Toolbar"
    ).GetGroups()
    for Group in CustomToolbars:
        Parameter = App.ParamGet(
            "User parameter:BaseApp/Workbench/Global/Toolbar/" + Group
        )
        ItemName = Parameter.GetString("Name")
        if ItemName == ToolBarName:
            return ToolBarName

    # add the ToolbarGroup in the FreeCAD Parameters
    WorkbenchToolbar = App.ParamGet(WorkbenchToolBarsParamPath + ToolBarName)

    # Set the name.
    WorkbenchToolbar.SetString("Name", ToolBarName)

    # Set the toolbar active
    WorkbenchToolbar.SetBool("Active", True)

    # add the commands
    for Button in ButtonList:
        WorkbenchToolbar.SetString(Button, "FreeCAD")
    # endregion

    App.saveParameter()
    return ToolBarName


def RemoveWorkBenchToolbars(Name: str, WorkBenchName: str = "Global") -> None:
    # Define the name for the ToolbarGroup in the FreeCAD Parameters
    ToolbarGroupName = WorkBenchName
    # Define the name for the toolbar
    ToolBarName = Name
    # define the parameter path for the toolbar
    ToolBarsParamPath = (
        "User parameter:BaseApp/Workbench/" + ToolbarGroupName + "/Toolbar/"
    )

    custom_toolbars = App.ParamGet(ToolBarsParamPath)
    custom_toolbars.RemGroup(ToolBarName)
    return


def ReturnXML_Value(
    path: str, ElementName: str, attribKey: str = "", attribValue: str = ""
):
    import os
    import xml.etree.ElementTree as ET

    # Passing the path of the
    # xml document to enable the
    # parsing process
    PackageXML = os.path.join(os.path.dirname(__file__), path)
    tree = ET.parse(PackageXML)
    # getting the parent tag of
    # the xml document
    root = tree.getroot()
    result = ""
    for child in root:
        if str(child.tag).split("}")[1] == ElementName:
            if attribKey != "" and attribValue != "":
                for key, value in child.attrib.items():
                    if key == attribKey and value == attribValue:
                        result = child.text
                        return result
            else:
                result = child.text
    return result


def ReturnXML_Value_Git(
    User="apebbers",
    Repository="FreeCAD-CommandTab",
    Branch="main",
    File="package.xml",
    ElementName: str = "",
    attribKey: str = "",
    attribValue: str = "",
    host="https://codeberg.org",
):
    # import requests_local as requests
    import xml.etree.ElementTree as ET
    from urllib import request

    result = None
    try:
        # Passing the path of the
        # xml document to enable the
        # parsing process
        url = f"{host}/{User}/{Repository}/{Branch}/{File}"
        if host == "https://codeberg.org":
           url = f"{host}/{User}/{Repository}/src/branch/{Branch}/{File}" 
        if host == "https://github.com":
            url = f"{host}/{User}/{Repository}/blob/{Branch}/{File}" 
        url = "https://raw.githubusercontent.com/APEbbers/FreeCAD-CommandTab/refs/heads/main/package.xml"
        response = request.urlopen(url, timeout=1.0)
        data = response.read()
        root: Element[str] = ET.fromstring(data)
        result = ""
        for child in root:
            if str(child.tag).split("}")[1] == ElementName:
                if attribKey != "" and attribValue != "":
                    for key, value in child.attrib.items():
                        if key == attribKey and value == attribValue:
                            result = child.text
                            return result
                else:
                    result = child.text
    except Exception:
        # raise e
        pass
    return result


def GetGitData(PrintErrors=False):
    GitInstalled = True
    import os

    try:
        import git
    except ImportError:
        GitInstalled = False

    commit = None
    branch = None
    Contributers = []
    result = [commit, branch, Contributers]

    git_root = os.path.join(os.path.dirname(__file__), ".git")
    if os.path.exists(git_root) is False:
        return result
    git_head = os.path.join(git_root, "HEAD")
    if os.path.exists(git_head) is False:
        return result

    # Read .git/HEAD file
    with open(git_head) as fd:
        head_ref = fd.read()

    # Find head file .git/HEAD (e.g. ref: ref/heads/master => .git/ref/heads/master)
    if not head_ref.startswith("ref: ") and PrintErrors is True:
        print(f"expected 'ref: path/to/head' in {git_head}")
        return result
    head_ref = head_ref[5:].strip()

    # Read commit id from head file
    head_path = os.path.join(git_root, head_ref)
    if os.path.exists(head_path) is False and PrintErrors is True:
        print(f"path {head_path} referenced from {git_head} does not exist")
        return result
    # Read the branch version
    branch = head_path.rsplit("/", 1)[1]
    with open(head_path) as fd:
        line = fd.readlines()[0]
        commit = line.strip()

    # If gitpython is installed, get the list of contributors
    if GitInstalled is True:
        repo = git.Repo(git_root)
        Git = repo.git
        List = Git.execute(
            ["git", "shortlog", "-sn", "-e", "--all"],
            as_process=False,
            stdout_as_string=True,
        )
        UserList = []
        for line in List.splitlines():
            Commits = str(line)[: len("  1418  ") - 1]
            Commits = int(Commits.strip())
            User = str(line)[len("  1418  ") - 1 :].split("<")[0].strip()
            email = (
                str(line)[len("  1418  ") - 1 :].split("<")[1].replace(">", "").strip()
            )

            UserList.append([Commits, User, email])

        tempList = []
        for i in range(len(UserList) - 1):
            User = UserList[i]
            if User[1] not in Contributers and User[1] != "pre-commit-ci[bot]":
                Contributers.append(User[1])
                tempList.append(User)
            if User[1] in Contributers:
                for j in range(len(UserList) - 1):
                    tempUser = UserList[j]
                    if tempUser[2] == User[2] and tempUser[0] > User[0]:
                        Contributers.pop()
                        if (
                            tempUser[1] not in Contributers
                            and tempUser[1] != "pre-commit-ci[bot]"
                        ):
                            Contributers.append(tempUser[1])

        # get the short commit id
        commit = repo.git.rev_parse(repo.head, short=True)

    result = [commit, branch, Contributers]
    return result


def TranslationsMapping(WorkBenchName: str, string: str):
    result = string

    partdesign_panel_mappings = {
        "Helpers": [
            ("Gui::TaskView::TaskWatcherCommands", "Helper Tools"),
            ("Workbench", "Part Design Helper"),
        ],
        "Modeling": [
            ("Gui::TaskView::TaskWatcherCommands", "Modeling Tools"),
            ("Workbench", "Part Design Modeling"),
        ],
        "Dress-Up": [
            ("Workbench", "Dress-Up Features"),
        ],
        "Patterns": [
            ("Workbench", "Transformation Features"),
        ],
    }
    if WorkBenchName == "PartDesignWorkbench" and string in partdesign_panel_mappings:
        for context, source_text in partdesign_panel_mappings[string]:
            translated_value = translate(context, source_text)
            if translated_value != source_text:
                return translated_value

    ListSpecialWB = [
        "Assembly4Workbench",
        "A2plusWorkbench",
    ]
    isSpecialWB = False
    for wb in ListSpecialWB:
        if wb == WorkBenchName:
            isSpecialWB = True

    if isSpecialWB is False:
        contextDict_Standard = {
            "WorkFeatureWorkbench": "Workbench",
            "SketcherWorkbench": "Workbench",
            "PartDesignWorkbench": "Workbench",
            "PartWorkbench": "Workbench",
            "SMWorkbench": "Workbench",
            "FrameWorkbench": "Workbench",
            "SurfaceWorkbench": "Workbench",
            "TechDrawWorkbench": "Workbench",
            "FemWorkbench": "Workbench",
            "GearWorkbench": "Workbench",
            "FastenersWorkbench": "Workbench",
            "SpreadsheetWorkbench": "Workbench",
            "InspectionWorkbench": "Workbench",
            "RenderWorkbench": "Workbench",
            "RobotWorkbench": "Workbench",
            "CfdOFWorkbench": "Workbench",
            "PlotWorkbench": "Workbench",
            "BillOfMaterialsWB": "Workbench",
            "DynamicDataWorkbench": "Workbench",
            "AssistantWorkbench": "Workbench",
            "TestWorkbench": "Workbench",
            "ThreadProfileWorkbench": "Workbench",
            "AssemblyWorkbench": "Workbench",
            "BIMWorkbench": "Workbench",
            "CAMWorkbench": "Workbench",
            "MaterialWorkbench": "Workbench",
            "Assembly3Workbench": "asm3",
        }
        try:
            context = contextDict_Standard[WorkBenchName]
        except Exception:
            context = "Workbench"
        result = translate(context, string)

    if WorkBenchName == "Assembly4Workbench":
        ListContext = [
            "Fasteners",
            "Commands",
            "Asm4_Help",
            "Commands1",
            "Asm4_showLcs",
            "Asm4_hideLcs",
        ]
        for i in range(len(ListContext)):
            context = ListContext[i]
            value = translate(context, string)
            if value != string:
                result = value

    if WorkBenchName == "A2plusWorkbench":
        ListContext = [
            "A2p_BoM",
            "A2plus",
            "A2plus_Constraints",
            "A2plus_searchConstraintConflicts",
        ]
        for i in range(len(ListContext)):
            context = ListContext[i]
            value = translate(context, string)
            if value != string:
                result = value

    return result


def _clean_visible_command_text(text: str, strip_ampersand: bool = True) -> str:
    result = str(text or "")
    if strip_ampersand is True:
        result = result.replace("&", "")
    return result


def _command_translation_contexts(CommandName: str) -> list:
    contexts = []
    command_name = str(CommandName or "")
    if command_name == "":
        return contexts

    contexts.append(command_name)

    normalized = "".join(
        [part for part in command_name.replace(",", "_").split("_") if part != ""]
    )
    if normalized != "":
        if command_name.startswith("Std_"):
            contexts.append(f"StdCmd{''.join(command_name.split('_')[1:])}")
        contexts.append(f"Cmd{normalized}")

    contexts.extend(["CommandGroup", "Workbench"])

    deduplicated = []
    for context in contexts:
        if context not in deduplicated and context not in ["", None]:
            deduplicated.append(context)
    return deduplicated


def ResolveCommandDisplayText(
    CommandName: str,
    menuText: str = "",
    actionText: str = "",
    stripAmpersand: bool = True,
):
    cleanedActionText = _clean_visible_command_text(actionText, stripAmpersand)
    if cleanedActionText != "":
        return cleanedActionText

    cleanedMenuText = _clean_visible_command_text(menuText, stripAmpersand)
    if cleanedMenuText == "":
        return _clean_visible_command_text(CommandName, stripAmpersand)

    for context in _command_translation_contexts(CommandName):
        translated = _clean_visible_command_text(
            translate(context, menuText), stripAmpersand
        )
        if translated not in ["", cleanedMenuText]:
            return translated

    return cleanedMenuText


def CommandDisplayText(
    CommandName: str,
    CommandInfo: dict | None = None,
    stripAmpersand: bool = True,
):
    info = CommandInfo if isinstance(CommandInfo, dict) else CommandInfoCorrections(CommandName)
    return ResolveCommandDisplayText(
        CommandName,
        str(info.get("menuText") or ""),
        str(info.get("ActionText") or ""),
        stripAmpersand,
    )


# # Add or update the dict for the CommandTab command panel
#         self.add_keys_nested_dict(
#             self.Dict_CommandTabCommandPanel,
#             ["workbenches", WorkBenchName, "toolbars", Toolbar, "order"],
#         )
def add_keys_nested_dict(dict, keys, default=1, endEmpty = False):
    """_summary_

    Args:
        dict (_type_): Enter dict to create or modify
        keys (_type_): Enter key or list of keys

    Returns:
        bool: True if a new dict is created or modified. Otherwise False
    """
    for key in keys:
        result = False
        if key not in dict:
            dict[key] = {}
            result = True
        dict = dict[key]
    try:
        if endEmpty is False:
            dict.setdefault(keys[-1], default)
    except Exception:
        pass
    return result


def returnDropDownCommands(command):
    Commands = []
    if command is not None:
        if len(command.getAction()) > 1:
            for i in range(len(command.getAction()) - 1):
                action = command.getAction()[i]
                if action is not None and (
                    action.icon() is not None and not action.icon().isNull()
                ):
                    Commands.append(
                        [
                            f"{command.getInfo()['name']}, {i}",
                            "actionIcon",
                            action.text(),
                            action.text(),
                        ]
                    )
    return Commands


def CommandInfoCorrections(CommandName):
    try:
        Command = Gui.Command.get(CommandName)
        if Command is not None:
            CommandInfo = Command.getInfo()
            specialUpdates = SPECIAL_COMMAND_INFO_UPDATES.get(CommandName, {})
            if isinstance(specialUpdates, dict):
                for key, value in specialUpdates.items():
                    if key in ["menuText", "toolTip", "statusTip", "ActionText", "DisplayText"]:
                        continue
                    CommandInfo[key] = value

            resolvedPixmapCommand = resolveCommandTabIconCommand(CommandName)
            if (
                commandtabUsesModernIconTheme() is True
                and resolvedPixmapCommand in CUSTOM_COMMAND_PIXMAPS
            ):
                CommandInfo["pixmap"] = CUSTOM_COMMAND_PIXMAPS[resolvedPixmapCommand]

            if commandtabUsesModernIconTheme() is True and CommandName in SPECIAL_PIXMAPS:
                CommandInfo["pixmap"] = SPECIAL_PIXMAPS[CommandName]

            CommandActionList = Command.getAction()
            if len(CommandActionList) > 0:
                CommandAction = CommandActionList[0]
                CommandInfo["ActionText"] = str(CommandAction.text() or "")
                if str(CommandAction.toolTip() or "").strip() != "":
                    CommandInfo["toolTip"] = str(CommandAction.toolTip() or "")
                if str(CommandAction.statusTip() or "").strip() != "":
                    CommandInfo["statusTip"] = str(CommandAction.statusTip() or "")
            else:
                CommandInfo["ActionText"] = ""

            menu_text = str(CommandInfo.get("menuText") or "")
            CommandInfo["ActionText"] = ResolveCommandDisplayText(
                CommandName,
                menu_text,
                CommandInfo["ActionText"],
                stripAmpersand=False,
            )
            CommandInfo["DisplayText"] = ResolveCommandDisplayText(
                CommandName,
                menu_text,
                CommandInfo["ActionText"],
            )

            if isinstance(specialUpdates, dict):
                for text_key in ["menuText", "toolTip", "statusTip"]:
                    if str(CommandInfo.get(text_key) or "").strip() == "":
                        value = str(specialUpdates.get(text_key) or "").strip()
                        if value != "":
                            CommandInfo[text_key] = value

            ChildCommands = returnDropDownCommands(Command)
            if len(ChildCommands) > 1:
                if not CommandInfo["menuText"].endswith("..."):
                    CommandInfo["menuText"] = CommandInfo["menuText"] + "..."
                if not CommandInfo["ActionText"].endswith("..."):
                    CommandInfo["ActionText"] = CommandInfo["ActionText"] + "..."

            return CommandInfo
        else:
            CommandInfo = {}
            CommandInfo["menuText"] = ""
            CommandInfo["toolTip"] = ""
            CommandInfo["whatsThis"] = ""
            CommandInfo["statusTip"] = ""
            CommandInfo["pixmap"] = ""
            CommandInfo["ActionText"] = ""
            CommandInfo["name"] = ""
    except Exception:
        CommandInfo = {}
        CommandInfo["menuText"] = ""
        CommandInfo["toolTip"] = ""
        CommandInfo["whatsThis"] = ""
        CommandInfo["statusTip"] = ""
        CommandInfo["pixmap"] = ""
        CommandInfo["ActionText"] = ""
        CommandInfo["name"] = ""
    return CommandInfo


def addMissingCommands(CommandList: list):
    MissingCommands = [
        [
            "Sketcher_NewSketch",  # commandname
            "Sketcher_NewSketch",  # iconname
            "Create sketch",  # menu text
            "SketcherWorkbench",  # workbench
        ],
        ["Draft_Line", "Draft_Line", "Line", "DraftWorkbench"],
        ["Draft_Move", "Draft_Move", "Move", "DraftWorkbench"],
        [
            "Draft_LayerManager",
            "Draft_LayerManager",
            "Manage layers...",
            "DraftWorkbench",
        ],
        ["Draft_Snap_Lock", "Draft_Snap_Lock", "Snap lock", "DraftWorkbench"],
        [
            "OpenSCAD_ReplaceObject",
            "OpenSCAD_ReplaceObject",
            "Replace Object",
            "OpenSCADWorkbench",
        ],
        [
            "Part_CheckGeometry",
            "Part_CheckGeometry",
            "Check Geometry",
            "OpenSCADWorkbench",
        ],
    ]

    CopyList = list(CommandList)
    existing_commands = {
        str(item[0] or "").strip()
        for item in CopyList
        if isinstance(item, (list, tuple)) and len(item) > 0
    }

    for MissingCommand in MissingCommands:
        command_name = str(MissingCommand[0] or "").strip()
        if command_name == "" or command_name in existing_commands:
            continue
        CopyList.append(MissingCommand)
        existing_commands.add(command_name)
    return CopyList


def returnQiCons_Commands(CommandName, pixmap=""):
    from PySide.QtGui import QIcon

    originalCommandName = normalizeCommandTabCommandName(CommandName)
    requestedPixmap = pixmap
    CommandName = resolveCommandTabIconCommand(CommandName)
    icon = QIcon()
    preferredPixmap = resolveCommandTabPreferredPixmap(CommandName, pixmap)

    if preferredPixmap != "" and preferredPixmap is not None:
        if os.path.exists(preferredPixmap):
            icon = QIcon(preferredPixmap)
        else:
            localPixmap = os.path.join(LOCAL_ICON_DIR, preferredPixmap)
            if os.path.exists(localPixmap):
                icon = QIcon(localPixmap)
            else:
                themedIcon = resolveBundledCommandTabThemeIcon(
                    os.path.basename(preferredPixmap)
                )
                if themedIcon.isNull() is False:
                    icon = themedIcon
                else:
                    themedIcon = resolveExternalCommandTabThemeIcon(
                        os.path.basename(preferredPixmap)
                    )
                    if themedIcon.isNull() is False:
                        icon = themedIcon
                    else:
                        icon = Gui.getIcon(preferredPixmap)
    else:
        try:
            Command = Gui.Command.get(CommandName)
            CommandInfo = Command.getInfo()
            pixmap = CommandInfo["pixmap"]
            themedIcon = resolveBundledCommandTabThemeIcon(os.path.basename(pixmap))
            if themedIcon.isNull() is False:
                return cropCommandTabIconTransparentMargins(
                    themedIcon, f"command:{originalCommandName}"
                )
            themedIcon = resolveExternalCommandTabThemeIcon(os.path.basename(pixmap))
            if themedIcon.isNull() is False:
                return cropCommandTabIconTransparentMargins(
                    themedIcon, f"command:{originalCommandName}"
                )
            if os.path.exists(pixmap):
                icon = QIcon(pixmap)
            else:
                localPixmap = os.path.join(LOCAL_ICON_DIR, pixmap)
                if os.path.exists(localPixmap):
                    icon = QIcon(localPixmap)
                else:
                    icon = Gui.getIcon(pixmap)
        except Exception:
            pass

    if icon is None or (icon is not None and icon.isNull()):
        themedCandidates = []
        for candidate in commandtabCommandIconCandidates(originalCommandName):
            themedCandidates.append(candidate)
        for candidate in themedCandidates:
            themedIcon = resolveBundledCommandTabThemeIcon(candidate)
            if themedIcon.isNull() is False:
                return cropCommandTabIconTransparentMargins(
                    themedIcon, f"command:{originalCommandName}"
                )
            themedIcon = resolveExternalCommandTabThemeIcon(candidate)
            if themedIcon.isNull() is False:
                return cropCommandTabIconTransparentMargins(
                    themedIcon, f"command:{originalCommandName}"
                )

    if icon is None or (icon is not None and icon.isNull()):
        try:
            if len(CommandName.split(", ")) > 1:
                CommandName_1 = CommandName.split(", ")[0]
                ActionNumber = int(CommandName.split(", ")[1])
                ParentCommand = Gui.Command.get(CommandName_1)
                if ParentCommand is not None:
                    action = ParentCommand.getAction()[ActionNumber]
                    icon = action.icon()
        except Exception:
            pass

    if icon is None or (icon is not None and icon.isNull()):
        try:
            Command = Gui.Command.get(CommandName)
            action = Command.getAction()[0]
            icon = action.icon()
        except Exception:
            return None
    return cropCommandTabIconTransparentMargins(
        icon,
        f"command:{originalCommandName}|{CommandName}|{preferredPixmap}|{requestedPixmap}",
    )


def CorrectGetToolbarItems(ToolbarItems: dict):
    newCommands = []

    if "Structure" in ToolbarItems:
        newCommands = ToolbarItems["Structure"]
        if "Part_Datums" not in newCommands:
            newCommands.append("Part_Datums")
            ToolbarItems.update({"Structure": newCommands})

    return ToolbarItems


def ShortCutTaken(ShortCut: str):
    ListWithCommands = Gui.Command.listByShortcut(ShortCut)

    if len(ListWithCommands) > 0:
        return True
    return False


def ReturnWrappedText(text: str, max_length: int = 50, max_Lines=0, returnList=False):
    import textwrap

    result = ""

    # Wrap the text as list
    wrapped_text = textwrap.wrap(text=text, width=max_length)

    # remove spaces at the end of each line
    for line in wrapped_text:
        line = textwrap.dedent(line)

    # remove any line that is more then> allowed
    if max_Lines > 0 and len(wrapped_text) > max_Lines:
        for i in range(max_Lines, len(wrapped_text)):
            try:
                wrapped_text.pop(i)
            except Exception:
                continue

    # return the desired result
    if returnList is False:
        result = "\n".join(wrapped_text)
    else:
        result = wrapped_text
    # print(result)
    return result


def AddToClipboard(Text):
    # import subprocess
    # import platform
    from PySide import QtWidgets

    # cmd = "clip" if platform.system() == "Windows" else "pbcopy"
    # subprocess.run(cmd, input=Text, text=True, shell=True)
    
    clipboard = QtWidgets.QApplication.clipboard()
    clipboard.setText(Text)


def checkFreeCADVersion(main: int, sub: int, patch: int, git: int):
    """Checks if the FreeCAD version is equal or newer than the given version number.

    Args:
        main (int): Main version number
        sub (int): Secundair version number
        patch (int): Patch number
        git (int): gitnumber

    Returns:
        True if the FreeCAD version is equal or higher than the given version number.
    """    
    version = App.Version()

    if main <= int(version[0]):
        if sub <= int(version[1]):
            if patch <= int(version[2]):
                git_version = int(version[3].split(" ")[0])
                if git <= git_version:
                    return True

    return False
