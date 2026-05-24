struct CommandTabPanelEntry
{
    QString id;
    QString title;
    QString sourceType;
    QString sourceWorkbenchId;
    QVector<CommandTabCommandEntry> commands;
};

struct CommandTabWorkbenchEntry
{
    QString id;
    QString title;
    QString iconPath;
    QVector<CommandTabPanelEntry> panels;
};

struct CommandTabWorkbenchVisibilityEntry
{
    QString id;
    QString title;
    QString iconPath;
    bool visible = true;
};

struct CommandTabPanelVisibilityEntry
{
    QString workbenchId;
    QString workbenchTitle;
    QString id;
    QString title;
    bool visible = true;
};

struct CommandTabDropdownDefinitionEntry
{
    QString id;
    QString text;
    QVector<CommandTabCommandEntry> commands;
};

struct CommandTabSettingsState
{
    bool preferNativeCommandTab = true;
    bool nativeCommandTabWarmup = false;
    bool modernCommandTabStyleEnabled = true;
    bool hideMenuBarInNativeMode = true;
    QString nativeThemeMode = QStringLiteral("auto");
    bool compactPanelLayout = true;
    int compactPanelSpacing = 1;
    int compactButtonPadding = 3;
    bool panelDropdownModeEnabled = false;
    bool panelDropdownPrimaryRecent = false;
    int panelDropdownRecentToolCount = 2;
    int panelDropdownPopupColumns = 4;
    bool panelDropdownPopupShowText = false;
    int panelDropdownPopupIconSize = 20;
    int displayScalePercent = 100;
    int headerScalePercent = 100;
    int commandtabScalePercent = 100;
    bool showIconTextSmall = true;
    bool showIconTextMedium = true;
    bool showIconTextLarge = true;
    int iconOnlySizeSmall = 20;
    int iconOnlySizeMedium = 24;
    int iconOnlySizeLarge = 32;
    bool showSketcherGrid = true;
    bool snapSketcherGrid = true;
    bool customMainColorsEnabled = false;
    QString customMainBackgroundColor;
    QString customMainTextColor;
    QString customRibbonPrimaryColor;
    QString customRibbonSecondaryColor;
    QString customRibbonAccentColor;
    bool ribbonAutoHide = false;
    int ribbonAutoHideDelayMs = 1500;
    bool ribbonHoverTab = false;
    bool tabClickPopupMode = false;
    bool viewportColorsEnabled = false;
    QString viewportBackgroundStyle = QStringLiteral("linear");
    QString viewportBgTopColor;
    QString viewportBgMidColor;
    QString viewportBgBottomColor;
    QString viewportBgAccentColor;
    bool gridColorEnabled = false;
    QString viewportGridColor;
};

struct NativeThemeConfig
{
    QString themeMode = QStringLiteral("auto");
    bool modernStyleEnabled = true;
    QJsonObject rawTheme;
};

struct CommandTabModel
{
    QString activeWorkbenchId;
    QVector<CommandTabCommandEntry> quickAccess;
    QVector<CommandTabWorkbenchEntry> workbenches;
    QVector<CommandTabWorkbenchVisibilityEntry> workbenchVisibility;
    QVector<CommandTabPanelVisibilityEntry> panelVisibility;
    QVector<CommandTabDropdownDefinitionEntry> dropdownDefinitions;
    CommandTabSettingsState settings;
    struct CommandTabTheme {
        bool isDark = true;
        bool forceTextColor = false;
        QColor shellBackground = QColor("#1e2329");  // Fond principal du shell ruban (dark base).
        QColor shellBorder = QColor("#4b5563");  // Bordure globale du shell ruban.
        QColor panelCardBackground = QColor("#262c34");  // Fond des cartes de panneau.
        QColor panelCardBorder = QColor("#4b5563");  // Bordure des cartes de panneau.
        QColor panelFooterBackground = QColor("#222830");  // Fond footer des panneaux.
        QColor panelFooterBorder = QColor("#3a4452");  // Bordure footer des panneaux.
        QColor panelBodyTop = QColor("#323a45");  // Couleur haute du corps de panneau.
        QColor panelBodyBottom = QColor("#252c34");  // Couleur basse du corps de panneau.
        QColor panelBodyAccent = QColor("#6f8296");  // Accent principal panneau/ruban.
        QColor titleText = QColor("#d7dde8");  // Texte titres/labels principaux.
        QColor separator = QColor("#7a8594");  // Séparateurs visuels.
        QColor quickBackground = QColor("#1e2329");  // Fond quick access toolbar.
        QColor quickBorder = QColor("#4b5563");  // Bordure quick access toolbar.
        QColor quickHoverBackground = QColor("#303845");  // Fond hover quick access.
        QColor quickHoverBorder = QColor("#7a8fa5");  // Bordure hover quick access.
        QColor tabBackground = QColor("#272e37");  // Fond onglets inactifs.
        QColor tabHoverBackground = QColor("#323b46");  // Fond onglets au survol.
        QColor tabSelectedBackground = QColor("#3b4653");  // Fond onglet actif/sélectionné.
        QColor tabBorder = QColor("#4b5563");  // Bordure onglets.
        QColor tabSelectedBorder = QColor("#6f7e90");  // Bordure onglet sélectionné.
        QColor tabAccent = QColor("#6f8296");  // Accent graphique des onglets.
        QColor buttonIdleTop = QColor("#2a313a");  // Dégradé haut bouton au repos.
        QColor buttonIdleBottom = QColor("#262d36");  // Dégradé bas bouton au repos.
        QColor buttonActiveTop = QColor("#384350");  // Dégradé haut bouton actif/hover.
        QColor buttonActiveBottom = QColor("#313b47");  // Dégradé bas bouton actif/hover.
        QColor buttonPressedTop = QColor("#252d37");  // Dégradé haut bouton pressé.
        QColor buttonPressedBottom = QColor("#212933");  // Dégradé bas bouton pressé.
        QColor buttonBorder = QColor("#4d5868");  // Bordure bouton au repos.
        QColor buttonActiveBorder = QColor("#879cb3");  // Bordure bouton actif.
        QColor buttonFocusBorder = QColor("#9bb1c8");  // Bordure focus clavier.
        QColor buttonText = QColor("#d7dde8");  // Texte des boutons.
        QColor buttonPlate = QColor("#ffffff");  // Plaque interne des boutons/icônes.
    } theme;
};

QColor parseThemeColor(const QJsonObject& object, const QString& key, const QColor& fallback)
{
    const QString value = object.value(key).toString();
    const QColor color(value);
    return color.isValid() ? color : fallback;
}

QString normalizedColorHex(QString value)
{
    value = value.trimmed();
    if (value.isEmpty()) {
        return QString();
    }
    const QColor color(value);
    if (!color.isValid()) {
        return QString();
    }
    return color.name(QColor::HexRgb);
}

QColor withAlpha(const QColor& color, int alpha)
{
    QColor tinted(color);
    tinted.setAlpha(std::clamp(alpha, 0, 255));
    return tinted;
}

QColor blendColors(const QColor& a, const QColor& b, qreal ratio)
{
    const qreal clamped = std::clamp(ratio, 0.0, 1.0);
    return QColor::fromRgbF(
        static_cast<float>(a.redF() * (1.0 - clamped) + b.redF() * clamped),
        static_cast<float>(a.greenF() * (1.0 - clamped) + b.greenF() * clamped),
        static_cast<float>(a.blueF() * (1.0 - clamped) + b.blueF() * clamped),
        static_cast<float>(a.alphaF() * (1.0 - clamped) + b.alphaF() * clamped)
    );
}

QColor modernAccentColor(const QColor& accentSeed, const QColor& context, bool darkMode)
{
    if (!accentSeed.isValid()) {
        return accentSeed;
    }

    QColor toned = accentSeed;
    float hue = -1.0f;
    float saturation = 0.0f;
    float value = 0.0f;
    float alpha = 1.0f;
    toned.getHsvF(&hue, &saturation, &value, &alpha);
    if (hue < 0.0f) {
        // Teinte absente (gris): ramène simplement la couleur vers le contexte.
        return blendColors(toned, context, darkMode ? 0.28 : 0.40);
    }

    const qreal saturationFactor = darkMode ? 0.42 : 0.32;
    const qreal valueFactor = darkMode ? 0.98 : 0.88;
    toned.setHsvF(
        hue,
        std::clamp(saturation * static_cast<float>(saturationFactor), 0.0f, 1.0f),
        std::clamp(value * static_cast<float>(valueFactor), 0.0f, 1.0f),
        std::clamp(alpha, 0.0f, 1.0f)
    );
    return blendColors(toned, context, darkMode ? 0.30 : 0.40);
}

qreal colorLuminanceApprox(const QColor& color)
{
    if (!color.isValid()) {
        return 0.0;
    }
    return
        0.2126 * color.redF()
        + 0.7152 * color.greenF()
        + 0.0722 * color.blueF();
}

qreal colorContrastRatioApprox(const QColor& first, const QColor& second)
{
    const qreal firstLuminance = colorLuminanceApprox(first);
    const qreal secondLuminance = colorLuminanceApprox(second);
    const qreal lighter = std::max(firstLuminance, secondLuminance);
    const qreal darker = std::min(firstLuminance, secondLuminance);
    return (lighter + 0.05) / (darker + 0.05);
}

QColor ensureReadableTextColor(
    const QColor& background,
    const QColor& preferred,
    qreal minimumContrast = 4.2,
    const QColor& darkCandidate = QColor(QStringLiteral("#17212b")),  // Fallback texte sombre.
    const QColor& lightCandidate = QColor(QStringLiteral("#f5f7fb"))  // Fallback texte clair.
)
{
    if (background.isValid()) {
        const QColor candidate = preferred.isValid() ? preferred : lightCandidate;
        if (colorContrastRatioApprox(candidate, background) >= minimumContrast) {
            return candidate;
        }
    }

    const qreal darkContrast = colorContrastRatioApprox(darkCandidate, background);
    const qreal lightContrast = colorContrastRatioApprox(lightCandidate, background);
    if (darkContrast > lightContrast) {
        return darkCandidate;
    }
    if (lightContrast > 0.0) {
        return lightCandidate;
    }
    return preferred.isValid() ? preferred : QColor(QStringLiteral("#f5f7fb"));  // Fallback ultime: texte clair.
}

[[maybe_unused]] QColor tonedReadableTextColor(
    const QColor& background,
    const QColor& baseText,
    qreal toneMix,
    qreal minimumContrast = 3.0
)
{
    const QColor toned = blendColors(baseText, background, std::clamp(toneMix, 0.0, 0.72));
    return ensureReadableTextColor(background, toned, minimumContrast);
}

bool inferDarkThemeFromSurfaces(const CommandTabModel::CommandTabTheme& theme)
{
    qreal accumulated = 0.0;
    int sampleCount = 0;
    const QVector<QColor> probes {
        theme.shellBackground,
        theme.panelCardBackground,
        theme.panelBodyBottom,
        theme.quickBackground,
        theme.tabBackground,
    };
    for (const QColor& probe : probes) {
        if (!probe.isValid()) {
            continue;
        }
        accumulated += colorLuminanceApprox(probe);
        ++sampleCount;
    }
    if (sampleCount <= 0) {
        return theme.isDark;
    }
    return (accumulated / static_cast<qreal>(sampleCount)) < 0.49;
}

QString normalizedThemeMode(QString value, bool allowAuto = true)
{
    value = value.trimmed().toLower();
    if (value == QStringLiteral("dark") || value == QStringLiteral("light")) {
        return value;
    }
    return allowAuto ? QStringLiteral("auto") : QString();
}

QString paletteResolvedThemeMode()
{
    if (qApp != nullptr) {
        const QPalette palette = qApp->palette();
        qreal accumulated = 0.0;
        int sampleCount = 0;
        const QVector<QColor> probes {
            palette.window().color(),
            palette.base().color(),
            palette.button().color(),
        };
        for (const QColor& probe : probes) {
            if (!probe.isValid()) {
                continue;
            }
            accumulated += colorLuminanceApprox(probe);
            ++sampleCount;
        }
        if (sampleCount > 0) {
            const qreal average = accumulated / static_cast<qreal>(sampleCount);
            return average < 0.49 ? QStringLiteral("dark") : QStringLiteral("light");
        }

        const QColor windowColor = palette.window().color();
        if (windowColor.isValid()) {
            return windowColor.lightness() < 128
                ? QStringLiteral("dark")
                : QStringLiteral("light");
        }
    }
    return QStringLiteral("dark");
}

NativeThemeConfig parseThemeConfig(const QJsonObject& object)
{
    NativeThemeConfig config;
    config.rawTheme = object;
    config.themeMode = normalizedThemeMode(object.value(QStringLiteral("themeMode")).toString(), true);
    config.modernStyleEnabled = object.value(QStringLiteral("modernStyleEnabled")).toBool(true);
    return config;
}

CommandTabModel::CommandTabTheme openDarkCommandTabTheme()
{
    CommandTabModel::CommandTabTheme theme;
    theme.isDark = true;
    theme.shellBackground = QColor("#1b2028");  // Theme dark: fond shell plus profond.
    theme.shellBorder = QColor("#3b4655");  // Theme dark: bordure shell adoucie.
    theme.panelCardBackground = QColor("#242b35");  // Theme dark: fond carte panneau.
    theme.panelCardBorder = QColor("#3c4859");  // Theme dark: bordure carte panneau.
    theme.panelFooterBackground = QColor("#202732");  // Theme dark: fond footer panneau.
    theme.panelFooterBorder = QColor("#344051");  // Theme dark: bordure footer panneau.
    theme.panelBodyTop = QColor("#2d3744");  // Theme dark: haut corps panneau.
    theme.panelBodyBottom = QColor("#222a34");  // Theme dark: bas corps panneau.
    theme.panelBodyAccent = QColor("#7aa8ff");  // Theme dark: accent principal.
    theme.titleText = QColor("#e6edf8");  // Theme dark: texte principal.
    theme.separator = QColor("#5f6d80");  // Theme dark: séparateurs.
    theme.quickBackground = QColor("#1d2430");  // Theme dark: fond quick access.
    theme.quickBorder = QColor("#3b4655");  // Theme dark: bordure quick access.
    theme.quickHoverBackground = QColor("#2b3544");  // Theme dark: hover quick access.
    theme.quickHoverBorder = QColor("#7090bf");  // Theme dark: bordure hover quick access.
    theme.tabBackground = QColor("#252e3a");  // Theme dark: fond onglets.
    theme.tabHoverBackground = QColor("#2f3a49");  // Theme dark: hover onglets.
    theme.tabSelectedBackground = QColor("#344255");  // Theme dark: onglet sélectionné.
    theme.tabBorder = QColor("#3f4b5d");  // Theme dark: bordure onglets.
    theme.tabSelectedBorder = QColor("#7aa8ff");  // Theme dark: bordure onglet sélectionné.
    theme.tabAccent = QColor("#7aa8ff");  // Theme dark: accent onglets.
    theme.buttonIdleTop = QColor("#2a3340");  // Theme dark: bouton repos (haut).
    theme.buttonIdleBottom = QColor("#242d39");  // Theme dark: bouton repos (bas).
    theme.buttonActiveTop = QColor("#364458");  // Theme dark: bouton actif (haut).
    theme.buttonActiveBottom = QColor("#2f3c4d");  // Theme dark: bouton actif (bas).
    theme.buttonPressedTop = QColor("#252f3d");  // Theme dark: bouton pressé (haut).
    theme.buttonPressedBottom = QColor("#202a37");  // Theme dark: bouton pressé (bas).
    theme.buttonBorder = QColor("#445164");  // Theme dark: bordure bouton.
    theme.buttonActiveBorder = QColor("#7aa8ff");  // Theme dark: bordure bouton actif.
    theme.buttonFocusBorder = QColor("#9cc2ff");  // Theme dark: bordure focus.
    theme.buttonText = QColor("#e6edf8");  // Theme dark: texte bouton.
    theme.buttonPlate = QColor("#ffffff");  // Theme dark: plaque bouton.
    return theme;
}

CommandTabModel::CommandTabTheme openLightCommandTabTheme()
{
    CommandTabModel::CommandTabTheme theme;
    theme.isDark = false;
    theme.shellBackground = QColor("#edf2f8");  // Theme light: fond shell.
    theme.shellBorder = QColor("#c2cedd");  // Theme light: bordure shell.
    theme.panelCardBackground = QColor("#f9fbff");  // Theme light: fond carte panneau.
    theme.panelCardBorder = QColor("#d6e0ee");  // Theme light: bordure carte panneau.
    theme.panelFooterBackground = QColor("#f1f6fd");  // Theme light: fond footer panneau.
    theme.panelFooterBorder = QColor("#d4deeb");  // Theme light: bordure footer panneau.
    theme.panelBodyTop = QColor("#ffffff");  // Theme light: haut corps panneau.
    theme.panelBodyBottom = QColor("#f4f8ff");  // Theme light: bas corps panneau.
    theme.panelBodyAccent = QColor("#3b82f6");  // Theme light: accent principal.
    theme.titleText = QColor("#111827");  // Theme light: texte principal.
    theme.separator = QColor("#8693a6");  // Theme light: séparateurs.
    theme.quickBackground = QColor("#f0f5fc");  // Theme light: fond quick access.
    theme.quickBorder = QColor("#d4deec");  // Theme light: bordure quick access.
    theme.quickHoverBackground = QColor("#e7f0ff");  // Theme light: hover quick access.
    theme.quickHoverBorder = QColor("#6d8fc4");  // Theme light: bordure hover quick access.
    theme.tabBackground = QColor("#f2f7ff");  // Theme light: fond onglets.
    theme.tabHoverBackground = QColor("#f7faff");  // Theme light: hover onglets.
    theme.tabSelectedBackground = QColor("#ffffff");  // Theme light: onglet sélectionné.
    theme.tabBorder = QColor("#d4deec");  // Theme light: bordure onglets.
    theme.tabSelectedBorder = QColor("#8fb2ef");  // Theme light: bordure onglet sélectionné.
    theme.tabAccent = QColor("#3b82f6");  // Theme light: accent onglets.
    theme.buttonIdleTop = QColor("#ffffff");  // Theme light: bouton repos (haut).
    theme.buttonIdleBottom = QColor("#f6f9ff");  // Theme light: bouton repos (bas).
    theme.buttonActiveTop = QColor("#ebf3ff");  // Theme light: bouton actif (haut).
    theme.buttonActiveBottom = QColor("#dfeafc");  // Theme light: bouton actif (bas).
    theme.buttonPressedTop = QColor("#dce8fb");  // Theme light: bouton pressé (haut).
    theme.buttonPressedBottom = QColor("#d2e1f7");  // Theme light: bouton pressé (bas).
    theme.buttonBorder = QColor("#c8d6ea");  // Theme light: bordure bouton.
    theme.buttonActiveBorder = QColor("#6f95d5");  // Theme light: bordure bouton actif.
    theme.buttonFocusBorder = QColor("#4f6f9e");  // Theme light: bordure focus.
    theme.buttonText = QColor("#111827");  // Theme light: texte bouton.
    theme.buttonPlate = QColor("#ffffff");  // Theme light: plaque bouton.
    return theme;
}

CommandTabModel::CommandTabTheme freecadDarkCommandTabTheme()
{
    CommandTabModel::CommandTabTheme theme;
    theme.isDark = true;
    theme.shellBackground = QColor("#1e2329");
    theme.shellBorder = QColor("#4b5563");
    theme.panelCardBackground = QColor("#262c34");
    theme.panelCardBorder = QColor("#4b5563");
    theme.panelFooterBackground = QColor("#222830");
    theme.panelFooterBorder = QColor("#3a4452");
    theme.panelBodyTop = QColor("#323a45");
    theme.panelBodyBottom = QColor("#252c34");
    theme.panelBodyAccent = QColor("#6f8296");
    theme.titleText = QColor("#d7dde8");
    theme.separator = QColor("#7a8594");
    theme.quickBackground = QColor("#1e2329");
    theme.quickBorder = QColor("#4b5563");
    theme.quickHoverBackground = QColor("#303845");
    theme.quickHoverBorder = QColor("#7a8fa5");
    theme.tabBackground = QColor("#272e37");
    theme.tabHoverBackground = QColor("#323b46");
    theme.tabSelectedBackground = QColor("#3b4653");
    theme.tabBorder = QColor("#4b5563");
    theme.tabSelectedBorder = QColor("#6f7e90");
    theme.tabAccent = QColor("#6f8296");
    theme.buttonIdleTop = QColor("#2a313a");
    theme.buttonIdleBottom = QColor("#262d36");
    theme.buttonActiveTop = QColor("#384350");
    theme.buttonActiveBottom = QColor("#313b47");
    theme.buttonPressedTop = QColor("#252d37");
    theme.buttonPressedBottom = QColor("#212933");
    theme.buttonBorder = QColor("#4d5868");
    theme.buttonActiveBorder = QColor("#879cb3");
    theme.buttonFocusBorder = QColor("#9bb1c8");
    theme.buttonText = QColor("#d7dde8");
    theme.buttonPlate = QColor("#ffffff");
    return theme;
}

CommandTabModel::CommandTabTheme freecadLightCommandTabTheme()
{
    CommandTabModel::CommandTabTheme theme;
    theme.isDark = false;
    theme.shellBackground = QColor("#eef1f5");  // FreeCAD light: fond shell.
    theme.shellBorder = QColor("#bcc5d1");  // FreeCAD light: bordure shell.
    theme.panelCardBackground = QColor("#ffffff");  // FreeCAD light: fond carte panneau.
    theme.panelCardBorder = QColor("#d5dce5");  // FreeCAD light: bordure carte panneau.
    theme.panelFooterBackground = QColor("#f1f5fa");  // FreeCAD light: fond footer panneau.
    theme.panelFooterBorder = QColor("#d4dbe5");  // FreeCAD light: bordure footer panneau.
    theme.panelBodyTop = QColor("#ffffff");  // FreeCAD light: haut corps panneau.
    theme.panelBodyBottom = QColor("#f6f9fc");  // FreeCAD light: bas corps panneau.
    theme.panelBodyAccent = QColor("#64798f");  // FreeCAD light: accent principal.
    theme.titleText = QColor("#000000");  // FreeCAD light: texte principal.
    theme.separator = QColor("#8b95a3");  // FreeCAD light: séparateurs.
    theme.quickBackground = QColor("#eef1f5");  // FreeCAD light: fond quick access.
    theme.quickBorder = QColor("#d5dce5");  // FreeCAD light: bordure quick access.
    theme.quickHoverBackground = QColor("#e8f2fc");  // FreeCAD light: hover quick access.
    theme.quickHoverBorder = QColor("#7188a0");  // FreeCAD light: bordure hover quick access.
    theme.tabBackground = QColor("#f1f5fa");  // FreeCAD light: fond onglets.
    theme.tabHoverBackground = QColor("#f8fafd");  // FreeCAD light: hover onglets.
    theme.tabSelectedBackground = QColor("#ffffff");  // FreeCAD light: onglet sélectionné.
    theme.tabBorder = QColor("#d4dbe5");  // FreeCAD light: bordure onglets.
    theme.tabSelectedBorder = QColor("#b5bfcd");  // FreeCAD light: bordure onglet sélectionné.
    theme.tabAccent = QColor("#64798f");  // FreeCAD light: accent onglets.
    theme.buttonIdleTop = QColor("#ffffff");  // FreeCAD light: bouton repos (haut).
    theme.buttonIdleBottom = QColor("#f7f9fc");  // FreeCAD light: bouton repos (bas).
    theme.buttonActiveTop = QColor("#edf4fc");  // FreeCAD light: bouton actif (haut).
    theme.buttonActiveBottom = QColor("#e1ecf8");  // FreeCAD light: bouton actif (bas).
    theme.buttonPressedTop = QColor("#deeaf7");  // FreeCAD light: bouton pressé (haut).
    theme.buttonPressedBottom = QColor("#d4e2f2");  // FreeCAD light: bouton pressé (bas).
    theme.buttonBorder = QColor("#cad4e1");  // FreeCAD light: bordure bouton.
    theme.buttonActiveBorder = QColor("#7e95ad");  // FreeCAD light: bordure bouton actif.
    theme.buttonFocusBorder = QColor("#5f748a");  // FreeCAD light: bordure focus.
    theme.buttonText = QColor("#000000");  // FreeCAD light: texte bouton.
    theme.buttonPlate = QColor("#fcfdff");  // FreeCAD light: plaque bouton.
    return theme;
}

bool isUsableThemeColor(const QColor& color)
{
    return color.isValid() && color.alpha() > 0;
}

QColor firstUsableThemeColor(const std::initializer_list<QColor>& colors)
{
    for (const QColor& color : colors) {
        if (isUsableThemeColor(color)) {
            return color;
        }
    }
    return QColor();
}

QColor paletteRoleColor(const QPalette& palette, QPalette::ColorRole role)
{
    const QColor active = palette.color(QPalette::Active, role);
    if (isUsableThemeColor(active)) {
        return active;
    }
    const QColor inactive = palette.color(QPalette::Inactive, role);
    if (isUsableThemeColor(inactive)) {
        return inactive;
    }
    const QColor disabled = palette.color(QPalette::Disabled, role);
    if (isUsableThemeColor(disabled)) {
        return disabled;
    }
    return QColor();
}

CommandTabModel::CommandTabTheme paletteBasedTheme(const QString& mode, bool modernStyleEnabled)
{
    const bool darkMode = mode != QStringLiteral("light");
    CommandTabModel::CommandTabTheme theme = modernStyleEnabled
        ? (darkMode ? openDarkCommandTabTheme() : openLightCommandTabTheme())
        : (darkMode ? freecadDarkCommandTabTheme() : freecadLightCommandTabTheme());
    theme.isDark = darkMode;

    if (qApp == nullptr) {
        return theme;
    }

    const QPalette palette = qApp->palette();
    const QColor shellBackground = firstUsableThemeColor({
        paletteRoleColor(palette, QPalette::Window),
        theme.shellBackground,
    });
    const QColor panelCardBackground = firstUsableThemeColor({
        paletteRoleColor(palette, QPalette::Button),
        paletteRoleColor(palette, QPalette::Base),
        theme.panelCardBackground,
    });
    const QColor panelFooterBackground = firstUsableThemeColor({
        paletteRoleColor(palette, QPalette::AlternateBase),
        paletteRoleColor(palette, QPalette::Window),
        theme.panelFooterBackground,
    });
    const QColor panelBodyTop = firstUsableThemeColor({
        paletteRoleColor(palette, QPalette::Button),
        panelCardBackground,
        theme.panelBodyTop,
    });
    const QColor panelBodyBottom = firstUsableThemeColor({
        paletteRoleColor(palette, QPalette::Base),
        panelCardBackground,
        theme.panelBodyBottom,
    });
    const QColor borderBase = firstUsableThemeColor({
        paletteRoleColor(palette, QPalette::Mid),
        paletteRoleColor(palette, QPalette::Midlight),
        paletteRoleColor(palette, QPalette::Dark),
        theme.shellBorder,
    });
    const QColor accentSeed = firstUsableThemeColor({
        paletteRoleColor(palette, QPalette::Highlight),
        paletteRoleColor(palette, QPalette::Link),
        theme.tabAccent,
    });
    const QColor accent = modernAccentColor(accentSeed, borderBase, darkMode);
    const QColor hoverSeedRaw = firstUsableThemeColor({
        paletteRoleColor(palette, QPalette::Highlight),
        accentSeed,
        theme.quickHoverBackground,
    });
    const QColor hoverSeed = blendColors(hoverSeedRaw, accent, darkMode ? 0.50 : 0.56);
    const QColor quickBackground = firstUsableThemeColor({
        paletteRoleColor(palette, QPalette::Button),
        shellBackground,
        theme.quickBackground,
    });
    const QColor tabBackground = firstUsableThemeColor({
        paletteRoleColor(palette, QPalette::Button),
        panelFooterBackground,
        theme.tabBackground,
    });
    const QColor tabSelectedBackground = firstUsableThemeColor({
        paletteRoleColor(palette, QPalette::Base),
        panelBodyTop,
        theme.tabSelectedBackground,
    });
    const QColor textSeed = firstUsableThemeColor({
        paletteRoleColor(palette, QPalette::ButtonText),
        paletteRoleColor(palette, QPalette::WindowText),
        paletteRoleColor(palette, QPalette::Text),
        darkMode ? QColor(QStringLiteral("#d7dde8")) : QColor(QStringLiteral("#000000")),
    });
    const QColor primaryText = ensureReadableTextColor(panelFooterBackground, textSeed, 4.5);

    theme.shellBackground = shellBackground;
    theme.shellBorder = borderBase;
    theme.panelCardBackground = panelCardBackground;
    theme.panelCardBorder = blendColors(borderBase, panelCardBackground, darkMode ? 0.22 : 0.34);
    theme.panelFooterBackground = panelFooterBackground;
    theme.panelFooterBorder = blendColors(borderBase, panelFooterBackground, darkMode ? 0.24 : 0.36);
    theme.panelBodyTop = panelBodyTop;
    theme.panelBodyBottom = panelBodyBottom;
    theme.panelBodyAccent = accent;
    theme.titleText = primaryText;
    theme.separator = blendColors(borderBase, accent, darkMode ? 0.16 : 0.10);
    theme.quickBackground = quickBackground;
    theme.quickBorder = blendColors(borderBase, quickBackground, darkMode ? 0.18 : 0.30);
    theme.quickHoverBackground = blendColors(quickBackground, hoverSeed, darkMode ? 0.30 : 0.22);
    theme.quickHoverBorder = blendColors(borderBase, accent, darkMode ? 0.28 : 0.16);
    theme.tabBackground = tabBackground;
    theme.tabHoverBackground = blendColors(tabBackground, hoverSeed, darkMode ? 0.26 : 0.20);
    theme.tabSelectedBackground = tabSelectedBackground;
    theme.tabBorder = blendColors(borderBase, tabBackground, darkMode ? 0.20 : 0.32);
    theme.tabSelectedBorder = blendColors(borderBase, accent, darkMode ? 0.34 : 0.22);
    theme.tabAccent = accent;
    theme.buttonIdleTop = blendColors(panelCardBackground, shellBackground, darkMode ? 0.10 : 0.08);
    theme.buttonIdleBottom = blendColors(panelCardBackground, panelBodyBottom, darkMode ? 0.24 : 0.14);
    theme.buttonActiveTop = blendColors(theme.buttonIdleTop, hoverSeed, darkMode ? 0.34 : 0.24);
    theme.buttonActiveBottom = blendColors(theme.buttonIdleBottom, hoverSeed, darkMode ? 0.28 : 0.20);
    theme.buttonPressedTop = blendColors(theme.buttonActiveTop, shellBackground, darkMode ? 0.20 : 0.12);
    theme.buttonPressedBottom = blendColors(theme.buttonActiveBottom, shellBackground, darkMode ? 0.24 : 0.14);
    theme.buttonBorder = blendColors(borderBase, panelCardBackground, darkMode ? 0.20 : 0.34);
    theme.buttonActiveBorder = blendColors(borderBase, accent, darkMode ? 0.36 : 0.22);
    theme.buttonFocusBorder = blendColors(
        accent,
        darkMode ? QColor(QStringLiteral("#ffffff")) : QColor(QStringLiteral("#0d1f33")),
        darkMode ? 0.22 : 0.12
    );
    theme.buttonText = primaryText;
    theme.buttonPlate = firstUsableThemeColor({
        paletteRoleColor(palette, QPalette::Base),
        panelBodyTop,
        theme.buttonPlate,
    });
    return theme;
}

QString cleanedWorkbenchTitle(QString value, const QString& fallbackId = QString())
{
    auto translateWorkbenchToken = [](const QString& token) -> QString {
        QString normalizedToken = token;
        normalizedToken = normalizedToken.replace(QLatin1Char('&'), QString()).trimmed();
        if (normalizedToken.isEmpty()) {
            return QString();
        }
        const QByteArray utf8Token = normalizedToken.toUtf8();
        QString translated = QCoreApplication::translate("Workbench", utf8Token.constData());
        translated = translated.replace(QLatin1Char('&'), QString()).simplified();
        return translated.isEmpty() ? QString() : translated;
    };

    value = value.replace(QLatin1Char('&'), QString()).trimmed();
    if (value.isEmpty()) {
        value = fallbackId.trimmed();
    }
    if (value.endsWith(QStringLiteral("Workbench"), Qt::CaseInsensitive)) {
        value.chop(QStringLiteral("Workbench").size());
    }
    QString cleaned;
    cleaned.reserve(value.size() + 8);
    for (int index = 0; index < value.size(); ++index) {
        const QChar current = value.at(index);
        if (
            index > 0
            && current.isUpper()
            && value.at(index - 1).isLetterOrNumber()
            && value.at(index - 1).isLower()
        ) {
            cleaned.append(QLatin1Char(' '));
        }
        cleaned.append(current);
    }
    cleaned = cleaned.simplified();
    if (cleaned.isEmpty()) {
        cleaned = fallbackId;
    }

    const QString translatedCleaned = translateWorkbenchToken(cleaned);
    if (!translatedCleaned.isEmpty()) {
        cleaned = translatedCleaned;
    } else {
        const QString fallbackNormalized = fallbackId.trimmed();
        if (!fallbackNormalized.isEmpty()) {
            QString fallbackCandidate = fallbackNormalized;
            if (fallbackCandidate.endsWith(QStringLiteral("Workbench"), Qt::CaseInsensitive)) {
                fallbackCandidate.chop(QStringLiteral("Workbench").size());
            }
            fallbackCandidate = fallbackCandidate.simplified();
            const QString translatedFallback = translateWorkbenchToken(fallbackCandidate);
            if (!translatedFallback.isEmpty()) {
                cleaned = translatedFallback;
            }
        }
    }

    return cleaned.isEmpty() ? fallbackId : cleaned;
}

CommandTabModel::CommandTabTheme parseTheme(const QJsonObject& object, const CommandTabModel::CommandTabTheme& fallback)
{
    CommandTabModel::CommandTabTheme theme = fallback;
    if (object.contains(QStringLiteral("isDark"))) {
        theme.isDark = object.value(QStringLiteral("isDark")).toBool(theme.isDark);
    }
    if (object.contains(QStringLiteral("forceTextColor"))) {
        theme.forceTextColor = object.value(QStringLiteral("forceTextColor")).toBool(theme.forceTextColor);
    }
    theme.shellBackground = parseThemeColor(object, QStringLiteral("shellBackground"), theme.shellBackground);
    theme.shellBorder = parseThemeColor(object, QStringLiteral("shellBorder"), theme.shellBorder);
    theme.panelCardBackground = parseThemeColor(object, QStringLiteral("panelCardBackground"), theme.panelCardBackground);
    theme.panelCardBorder = parseThemeColor(object, QStringLiteral("panelCardBorder"), theme.panelCardBorder);
    theme.panelFooterBackground = parseThemeColor(object, QStringLiteral("panelFooterBackground"), theme.panelFooterBackground);
    theme.panelFooterBorder = parseThemeColor(object, QStringLiteral("panelFooterBorder"), theme.panelFooterBorder);
    theme.panelBodyTop = parseThemeColor(object, QStringLiteral("panelBodyTop"), theme.panelBodyTop);
    theme.panelBodyBottom = parseThemeColor(object, QStringLiteral("panelBodyBottom"), theme.panelBodyBottom);
    theme.panelBodyAccent = parseThemeColor(object, QStringLiteral("panelBodyAccent"), theme.panelBodyAccent);
    theme.titleText = parseThemeColor(object, QStringLiteral("titleText"), theme.titleText);
    theme.separator = parseThemeColor(object, QStringLiteral("separator"), theme.separator);
    theme.quickBackground = parseThemeColor(object, QStringLiteral("quickBackground"), theme.quickBackground);
    theme.quickBorder = parseThemeColor(object, QStringLiteral("quickBorder"), theme.quickBorder);
    theme.quickHoverBackground = parseThemeColor(object, QStringLiteral("quickHoverBackground"), theme.quickHoverBackground);
    theme.quickHoverBorder = parseThemeColor(object, QStringLiteral("quickHoverBorder"), theme.quickHoverBorder);
    theme.tabBackground = parseThemeColor(object, QStringLiteral("tabBackground"), theme.tabBackground);
    theme.tabHoverBackground = parseThemeColor(object, QStringLiteral("tabHoverBackground"), theme.tabHoverBackground);
    theme.tabSelectedBackground = parseThemeColor(object, QStringLiteral("tabSelectedBackground"), theme.tabSelectedBackground);
    theme.tabBorder = parseThemeColor(object, QStringLiteral("tabBorder"), theme.tabBorder);
    theme.tabSelectedBorder = parseThemeColor(object, QStringLiteral("tabSelectedBorder"), theme.tabSelectedBorder);
    theme.tabAccent = parseThemeColor(object, QStringLiteral("tabAccent"), theme.tabAccent);
    theme.buttonIdleTop = parseThemeColor(object, QStringLiteral("buttonIdleTop"), theme.buttonIdleTop);
    theme.buttonIdleBottom = parseThemeColor(object, QStringLiteral("buttonIdleBottom"), theme.buttonIdleBottom);
    theme.buttonActiveTop = parseThemeColor(object, QStringLiteral("buttonActiveTop"), theme.buttonActiveTop);
    theme.buttonActiveBottom = parseThemeColor(object, QStringLiteral("buttonActiveBottom"), theme.buttonActiveBottom);
    theme.buttonPressedTop = parseThemeColor(object, QStringLiteral("buttonPressedTop"), theme.buttonPressedTop);
    theme.buttonPressedBottom = parseThemeColor(object, QStringLiteral("buttonPressedBottom"), theme.buttonPressedBottom);
    theme.buttonBorder = parseThemeColor(object, QStringLiteral("buttonBorder"), theme.buttonBorder);
    theme.buttonActiveBorder = parseThemeColor(object, QStringLiteral("buttonActiveBorder"), theme.buttonActiveBorder);
    theme.buttonFocusBorder = parseThemeColor(object, QStringLiteral("buttonFocusBorder"), theme.buttonFocusBorder);
    theme.buttonText = parseThemeColor(object, QStringLiteral("buttonText"), theme.buttonText);
    theme.buttonPlate = parseThemeColor(object, QStringLiteral("buttonPlate"), theme.buttonPlate);
    return theme;
}

CommandTabModel::CommandTabTheme parseTheme(const QJsonObject& object)
{
    return parseTheme(object, CommandTabModel::CommandTabTheme());
}

CommandTabModel::CommandTabTheme resolveThemeFromConfig(const NativeThemeConfig& config)
{
    QString effectiveMode = normalizedThemeMode(config.themeMode, true);
    const QString paletteMode = paletteResolvedThemeMode();
    if (effectiveMode == QStringLiteral("auto")) {
        // Auto must resolve to an actual palette mode, never stay as "auto".
        if (paletteMode == QStringLiteral("dark") || paletteMode == QStringLiteral("light")) {
            effectiveMode = paletteMode;
        } else {
            effectiveMode = QStringLiteral("dark");
        }
    }

    const CommandTabModel::CommandTabTheme paletteTheme = paletteBasedTheme(effectiveMode, config.modernStyleEnabled);
    CommandTabModel::CommandTabTheme resolvedTheme = parseTheme(config.rawTheme, paletteTheme);
    // Sécurise le mode final depuis les surfaces, pour éviter texte sombre sur fond sombre.
    resolvedTheme.isDark = inferDarkThemeFromSurfaces(resolvedTheme);
    return resolvedTheme;
}

CommandTabModel::CommandTabTheme applySettingsThemeOverrides(
    CommandTabModel::CommandTabTheme theme,
    const CommandTabSettingsState& settings
)
{
    const QColor customRibbonAccent(settings.customRibbonAccentColor.trimmed());
    if (!settings.customMainColorsEnabled) {
        if (!customRibbonAccent.isValid()) {
            return theme;
        }

        const bool darkMode = inferDarkThemeFromSurfaces(theme);
        const QColor lightAnchor(QStringLiteral("#ffffff"));
        const QColor darkAnchor(QStringLiteral("#0f141a"));
        const QColor borderSeed = theme.shellBorder.isValid()
            ? theme.shellBorder
            : (
                darkMode
                    ? blendColors(theme.shellBackground, lightAnchor, 0.20)
                    : blendColors(theme.shellBackground, darkAnchor, 0.16)
            );
        const QColor accent = customRibbonAccent;

        theme.isDark = darkMode;
        theme.panelBodyAccent = accent;
        theme.separator = blendColors(borderSeed, accent, darkMode ? 0.20 : 0.12);
        theme.quickHoverBorder = blendColors(borderSeed, accent, darkMode ? 0.30 : 0.20);
        theme.tabSelectedBorder = blendColors(borderSeed, accent, darkMode ? 0.38 : 0.24);
        theme.tabAccent = accent;
        theme.buttonActiveTop = blendColors(theme.buttonIdleTop, accent, darkMode ? 0.24 : 0.16);
        theme.buttonActiveBottom = blendColors(theme.buttonIdleBottom, accent, darkMode ? 0.20 : 0.14);
        theme.buttonPressedTop = blendColors(theme.buttonActiveTop, theme.shellBackground, darkMode ? 0.24 : 0.14);
        theme.buttonPressedBottom = blendColors(theme.buttonActiveBottom, theme.shellBackground, darkMode ? 0.28 : 0.18);
        theme.buttonActiveBorder = blendColors(borderSeed, accent, darkMode ? 0.36 : 0.24);
        theme.buttonFocusBorder = blendColors(accent, darkMode ? lightAnchor : darkAnchor, darkMode ? 0.20 : 0.10);
        return theme;
    }

    const QColor customBackground(settings.customMainBackgroundColor.trimmed());
    const QColor customText(settings.customMainTextColor.trimmed());
    const QColor customRibbonPrimary(settings.customRibbonPrimaryColor.trimmed());
    const QColor customRibbonSecondary(settings.customRibbonSecondaryColor.trimmed());
    if (
        !customBackground.isValid()
        && !customText.isValid()
        && !customRibbonPrimary.isValid()
        && !customRibbonSecondary.isValid()
        && !customRibbonAccent.isValid()
    ) {
        return theme;
    }

    // Keep "Main background" and "Ribbon primary" independently effective:
    // - shellBackground drives the global container background
    // - ribbonPrimarySurface drives ribbon bars/tabs and related derivations
    const QColor fallbackShellBackground = theme.shellBackground.isValid()
        ? theme.shellBackground
        : QColor(theme.isDark ? QStringLiteral("#1e2329") : QStringLiteral("#eef1f5"));
    const QColor shellBackground = customBackground.isValid()
        ? customBackground
        : (customRibbonPrimary.isValid() ? customRibbonPrimary : fallbackShellBackground);
    const QColor ribbonPrimarySurface = customRibbonPrimary.isValid()
        ? customRibbonPrimary
        : shellBackground;
    const bool darkMode = colorLuminanceApprox(
        blendColors(shellBackground, ribbonPrimarySurface, 0.55)
    ) < 0.49;
    const QColor lightAnchor(QStringLiteral("#ffffff"));
    const QColor darkAnchor(QStringLiteral("#0f141a"));
    const QColor panelCardBackground = customRibbonSecondary.isValid()
        ? customRibbonSecondary
        : (
            darkMode
                ? blendColors(ribbonPrimarySurface, lightAnchor, 0.13)
                : blendColors(ribbonPrimarySurface, lightAnchor, 0.62)
        );
    const QColor panelFooterBackground = darkMode
        ? blendColors(panelCardBackground, darkAnchor, 0.14)
        : blendColors(panelCardBackground, ribbonPrimarySurface, 0.10);
    const QColor panelBodyTop = darkMode
        ? blendColors(panelCardBackground, lightAnchor, 0.10)
        : blendColors(panelCardBackground, lightAnchor, 0.20);
    const QColor panelBodyBottom = darkMode
        ? blendColors(panelCardBackground, darkAnchor, 0.14)
        : blendColors(panelCardBackground, ribbonPrimarySurface, 0.08);
    const QColor borderSeed = darkMode
        ? blendColors(blendColors(shellBackground, ribbonPrimarySurface, 0.45), lightAnchor, 0.20)
        : blendColors(blendColors(shellBackground, ribbonPrimarySurface, 0.45), darkAnchor, 0.16);
    const QColor accentSeed = customRibbonAccent.isValid()
        ? customRibbonAccent
        : (theme.tabAccent.isValid()
            ? theme.tabAccent
            : QColor(QStringLiteral("#6f8296")));
    const QColor accent = customRibbonAccent.isValid()
        ? customRibbonAccent
        : modernAccentColor(accentSeed, borderSeed, darkMode);
    const QColor quickBackground = darkMode
        ? blendColors(ribbonPrimarySurface, panelCardBackground, 0.26)
        : blendColors(ribbonPrimarySurface, panelCardBackground, 0.44);
    const QColor tabBackground = darkMode
        ? blendColors(ribbonPrimarySurface, panelCardBackground, 0.44)
        : blendColors(panelCardBackground, ribbonPrimarySurface, 0.16);
    const QColor tabSelectedBackground = darkMode
        ? blendColors(tabBackground, lightAnchor, 0.24)
        : blendColors(tabBackground, lightAnchor, 0.52);
    const bool forceTextColor = customText.isValid();
    const QColor preferredText = forceTextColor
        ? customText
        : QColor(darkMode ? QStringLiteral("#dde4ef") : QStringLiteral("#111111"));
    const QColor primaryText = forceTextColor
        ? preferredText
        : ensureReadableTextColor(panelCardBackground, preferredText, 4.6);

    theme.isDark = darkMode;
    theme.forceTextColor = forceTextColor;
    theme.shellBackground = shellBackground;
    theme.shellBorder = borderSeed;
    theme.panelCardBackground = panelCardBackground;
    theme.panelCardBorder = blendColors(borderSeed, panelCardBackground, darkMode ? 0.22 : 0.36);
    theme.panelFooterBackground = panelFooterBackground;
    theme.panelFooterBorder = blendColors(borderSeed, panelFooterBackground, darkMode ? 0.26 : 0.38);
    theme.panelBodyTop = panelBodyTop;
    theme.panelBodyBottom = panelBodyBottom;
    theme.panelBodyAccent = accent;
    theme.titleText = forceTextColor ? preferredText : primaryText;
    theme.separator = blendColors(borderSeed, accent, darkMode ? 0.20 : 0.12);
    theme.quickBackground = quickBackground;
    theme.quickBorder = blendColors(borderSeed, quickBackground, darkMode ? 0.16 : 0.22);
    theme.quickHoverBackground = darkMode
        ? blendColors(quickBackground, lightAnchor, 0.18)
        : blendColors(quickBackground, accent, 0.16);
    theme.quickHoverBorder = blendColors(borderSeed, accent, darkMode ? 0.30 : 0.20);
    theme.tabBackground = tabBackground;
    theme.tabHoverBackground = darkMode
        ? blendColors(tabBackground, lightAnchor, 0.16)
        : blendColors(tabBackground, accent, 0.12);
    theme.tabSelectedBackground = tabSelectedBackground;
    theme.tabBorder = blendColors(borderSeed, tabBackground, darkMode ? 0.18 : 0.26);
    theme.tabSelectedBorder = blendColors(borderSeed, accent, darkMode ? 0.38 : 0.24);
    theme.tabAccent = accent;
    theme.buttonIdleTop = blendColors(panelCardBackground, panelBodyTop, darkMode ? 0.56 : 0.34);
    theme.buttonIdleBottom = blendColors(panelCardBackground, panelBodyBottom, darkMode ? 0.44 : 0.26);
    theme.buttonActiveTop = blendColors(theme.buttonIdleTop, accent, darkMode ? 0.24 : 0.16);
    theme.buttonActiveBottom = blendColors(theme.buttonIdleBottom, accent, darkMode ? 0.20 : 0.14);
    theme.buttonPressedTop = blendColors(
        theme.buttonActiveTop,
        blendColors(shellBackground, ribbonPrimarySurface, 0.40),
        darkMode ? 0.24 : 0.14
    );
    theme.buttonPressedBottom = blendColors(
        theme.buttonActiveBottom,
        blendColors(shellBackground, ribbonPrimarySurface, 0.40),
        darkMode ? 0.28 : 0.18
    );
    theme.buttonBorder = blendColors(borderSeed, panelCardBackground, darkMode ? 0.16 : 0.26);
    theme.buttonActiveBorder = blendColors(borderSeed, accent, darkMode ? 0.36 : 0.24);
    theme.buttonFocusBorder = blendColors(accent, darkMode ? lightAnchor : darkAnchor, darkMode ? 0.20 : 0.10);
    theme.buttonText = forceTextColor ? preferredText : primaryText;
    theme.buttonPlate = darkMode
        ? blendColors(panelBodyTop, lightAnchor, 0.06)
        : blendColors(panelBodyTop, lightAnchor, 0.72);
    return theme;
}

CommandTabModel::CommandTabTheme normalizeShellTheme(CommandTabModel::CommandTabTheme theme)
{
    const QColor lightModeText = QColor(QStringLiteral("#000000"));  // Texte canonique en mode light.
    const QColor darkModeText = QColor(QStringLiteral("#d7dde8"));  // Texte canonique en mode dark.
    const bool resolvedDarkMode = inferDarkThemeFromSurfaces(theme);
    theme.isDark = resolvedDarkMode;
    const QColor borderBase = theme.shellBorder.isValid()
        ? theme.shellBorder
        : QColor(resolvedDarkMode ? QStringLiteral("#4b5563") : QStringLiteral("#cad4e1"));  // Fallback bordure dark/light.
    const QColor accent = theme.tabAccent.isValid()
        ? theme.tabAccent
        : (theme.panelBodyAccent.isValid() ? theme.panelBodyAccent : QColor(QStringLiteral("#6f8296")));
    const QColor softBorder = blendColors(borderBase, accent, resolvedDarkMode ? 0.12 : 0.06);
    const QColor strongBorder = blendColors(borderBase, accent, resolvedDarkMode ? 0.22 : 0.12);
    const QColor defaultPreferredText = resolvedDarkMode ? darkModeText : lightModeText;
    const QColor preferredButtonText = theme.buttonText.isValid()
        ? theme.buttonText
        : defaultPreferredText;
    const QColor preferredTitleText = theme.titleText.isValid()
        ? theme.titleText
        : preferredButtonText;

    if (theme.forceTextColor) {
        theme.titleText = preferredTitleText;
        theme.buttonText = preferredButtonText;
    } else {
        theme.titleText = ensureReadableTextColor(theme.panelFooterBackground, preferredTitleText, 4.6);
        theme.buttonText = ensureReadableTextColor(theme.panelCardBackground, preferredButtonText, 4.6);
    }
    theme.shellBorder = softBorder;
    theme.panelCardBorder = blendColors(theme.panelCardBorder, softBorder, 0.55);
    theme.panelFooterBorder = blendColors(theme.panelFooterBorder, softBorder, 0.60);
    theme.quickBorder = blendColors(theme.quickBorder, softBorder, 0.65);
    theme.quickHoverBorder = strongBorder;
    theme.tabBorder = blendColors(theme.tabBorder, softBorder, 0.70);
    theme.tabSelectedBorder = blendColors(theme.tabSelectedBorder, strongBorder, 0.70);
    theme.tabAccent = accent;
    theme.separator = blendColors(theme.separator, softBorder, 0.75);
    theme.buttonBorder = blendColors(theme.buttonBorder, softBorder, 0.70);
    theme.buttonActiveBorder = strongBorder;
    theme.buttonFocusBorder = blendColors(theme.buttonFocusBorder, accent, 0.45);
    theme.panelBodyAccent = accent;
    return theme;
}

QString nativeHeaderLogoResource(const CommandTabModel::CommandTabTheme& theme)
{
    return theme.isDark
        ? QStringLiteral(":/theme/dark/logo")
        : QStringLiteral(":/theme/light/logo");
}

[[maybe_unused]] QString nativeMoreArrowResource(const CommandTabModel::CommandTabTheme& theme)
{
    return theme.isDark
        ? QStringLiteral(":/theme/dark/more_arrow")
        : QStringLiteral(":/theme/light/more_arrow");
}

bool looksLikeSeparatorCommandId(const QString& commandId)
{
    const QString normalized = commandId.trimmed().toLower();
    if (normalized.isEmpty()) {
        return false;
    }
    if (
        normalized == QStringLiteral("0")
        || normalized == QStringLiteral("|")
        || normalized == QStringLiteral("-")
        || normalized == QStringLiteral("--")
        || normalized == QStringLiteral("---")
        || normalized == QStringLiteral("separator")
    ) {
        return true;
    }
    return normalized.startsWith(QStringLiteral("separator_"))
        || normalized.endsWith(QStringLiteral("_separator"))
        || normalized.contains(QStringLiteral("_separator_"));
}

CommandTabCommandEntry parseCommand(const QJsonObject& object)
{
    CommandTabCommandEntry command;
    command.type = object.value(QStringLiteral("type")).toString(QStringLiteral("command"));
    command.id = object.value(QStringLiteral("id")).toString();
    command.text = object.value(QStringLiteral("text")).toString();
    command.iconPath = object.value(QStringLiteral("iconPath")).toString();
    command.shortcut = object.value(QStringLiteral("shortcut")).toString();
    command.size = object.value(QStringLiteral("size")).toString(QStringLiteral("small"));
    const QString normalizedSize = command.size.trimmed().toLower();
    if (
        normalizedSize != QStringLiteral("small")
        && normalizedSize != QStringLiteral("medium")
        && normalizedSize != QStringLiteral("large")
    ) {
        command.size = QStringLiteral("small");
    } else {
        command.size = normalizedSize;
    }
    if (looksLikeSeparatorCommandId(command.id)) {
        command.type = QStringLiteral("separator");
        command.size = QStringLiteral("small");
    }
    command.textVisible = object.value(QStringLiteral("textVisible")).toBool(true);
    if (object.contains(QStringLiteral("textVisibilityExplicit"))) {
        command.textVisibilityExplicit =
            object.value(QStringLiteral("textVisibilityExplicit")).toBool(false);
    } else {
        // Backward compatibility: older payloads used only textVisible to force
        // per-command behavior.
        command.textVisibilityExplicit = object.contains(QStringLiteral("textVisible"));
    }
    command.sourceWorkbenchId = object.value(QStringLiteral("sourceWorkbenchId")).toString();
    command.sourceToolbarTitle = object.value(QStringLiteral("sourceToolbarTitle")).toString();
    const auto menuCommands = object.value(QStringLiteral("menuCommands")).toArray();
    for (const auto& menuCommandValue : menuCommands) {
        if (!menuCommandValue.isObject()) {
            continue;
        }
        command.menuCommands.push_back(parseCommand(menuCommandValue.toObject()));
    }
    return command;
}

CommandTabPanelEntry parsePanel(const QJsonObject& object)
{
    CommandTabPanelEntry panel;
    panel.id = object.value(QStringLiteral("id")).toString();
    panel.title = object.value(QStringLiteral("title")).toString(panel.id);
    panel.sourceType = object.value(QStringLiteral("sourceType")).toString(QStringLiteral("toolbar"));
    panel.sourceWorkbenchId = object.value(QStringLiteral("sourceWorkbenchId")).toString();

    const auto commands = object.value(QStringLiteral("commands")).toArray();
    for (const auto& commandValue : commands) {
        if (!commandValue.isObject()) {
            continue;
        }
        panel.commands.push_back(parseCommand(commandValue.toObject()));
    }
    return panel;
}

CommandTabWorkbenchEntry parseWorkbench(const QJsonObject& object)
{
    CommandTabWorkbenchEntry workbench;
    workbench.id = object.value(QStringLiteral("id")).toString();
    workbench.title = cleanedWorkbenchTitle(
        object.value(QStringLiteral("title")).toString(workbench.id),
        workbench.id
    );
    workbench.iconPath = object.value(QStringLiteral("iconPath")).toString();

    const auto panels = object.value(QStringLiteral("panels")).toArray();
    for (const auto& panelValue : panels) {
        if (!panelValue.isObject()) {
            continue;
        }

        CommandTabPanelEntry panel = parsePanel(panelValue.toObject());
        if (!panel.commands.isEmpty()) {
            workbench.panels.push_back(panel);
        }
    }
    return workbench;
}

QJsonObject commandToJsonObject(const CommandTabCommandEntry& command)
{
    QJsonObject object;
    object.insert(QStringLiteral("type"), command.type);
    object.insert(QStringLiteral("id"), command.id);
    object.insert(QStringLiteral("text"), command.text);
    object.insert(QStringLiteral("iconPath"), command.iconPath);
    if (!command.shortcut.isEmpty()) {
        object.insert(QStringLiteral("shortcut"), command.shortcut);
    }
    object.insert(QStringLiteral("size"), command.size);
    object.insert(QStringLiteral("textVisible"), command.textVisible);
    object.insert(QStringLiteral("textVisibilityExplicit"), command.textVisibilityExplicit);
    if (!command.sourceWorkbenchId.isEmpty()) {
        object.insert(QStringLiteral("sourceWorkbenchId"), command.sourceWorkbenchId);
    }
    if (!command.sourceToolbarTitle.isEmpty()) {
        object.insert(QStringLiteral("sourceToolbarTitle"), command.sourceToolbarTitle);
    }
    if (!command.menuCommands.isEmpty()) {
        QJsonArray menuCommands;
        for (const auto& menuCommand : command.menuCommands) {
            menuCommands.append(commandToJsonObject(menuCommand));
        }
        object.insert(QStringLiteral("menuCommands"), menuCommands);
    }
    return object;
}

QJsonObject panelToJsonObject(const CommandTabPanelEntry& panel)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), panel.id);
    object.insert(QStringLiteral("title"), panel.title);
    object.insert(QStringLiteral("sourceType"), panel.sourceType);
    if (!panel.sourceWorkbenchId.isEmpty()) {
        object.insert(QStringLiteral("sourceWorkbenchId"), panel.sourceWorkbenchId);
    }
    QJsonArray commands;
    for (const auto& command : panel.commands) {
        commands.append(commandToJsonObject(command));
    }
    object.insert(QStringLiteral("commands"), commands);
    return object;
}

QJsonObject workbenchToJsonObject(const CommandTabWorkbenchEntry& workbench)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), workbench.id);
    object.insert(QStringLiteral("title"), workbench.title);
    if (!workbench.iconPath.isEmpty()) {
        object.insert(QStringLiteral("iconPath"), workbench.iconPath);
    }
    QJsonArray panels;
    for (const auto& panel : workbench.panels) {
        panels.append(panelToJsonObject(panel));
    }
    object.insert(QStringLiteral("panels"), panels);
    return object;
}

QJsonObject themeToJsonObject(const CommandTabModel::CommandTabTheme& theme)
{
    QJsonObject object;
    object.insert(QStringLiteral("isDark"), theme.isDark);
    object.insert(QStringLiteral("forceTextColor"), theme.forceTextColor);
    object.insert(QStringLiteral("shellBackground"), theme.shellBackground.name(QColor::HexRgb));
    object.insert(QStringLiteral("shellBorder"), theme.shellBorder.name(QColor::HexRgb));
    object.insert(QStringLiteral("panelCardBackground"), theme.panelCardBackground.name(QColor::HexRgb));
    object.insert(QStringLiteral("panelCardBorder"), theme.panelCardBorder.name(QColor::HexRgb));
    object.insert(QStringLiteral("panelFooterBackground"), theme.panelFooterBackground.name(QColor::HexRgb));
    object.insert(QStringLiteral("panelFooterBorder"), theme.panelFooterBorder.name(QColor::HexRgb));
    object.insert(QStringLiteral("panelBodyTop"), theme.panelBodyTop.name(QColor::HexRgb));
    object.insert(QStringLiteral("panelBodyBottom"), theme.panelBodyBottom.name(QColor::HexRgb));
    object.insert(QStringLiteral("panelBodyAccent"), theme.panelBodyAccent.name(QColor::HexRgb));
    object.insert(QStringLiteral("titleText"), theme.titleText.name(QColor::HexRgb));
    object.insert(QStringLiteral("separator"), theme.separator.name(QColor::HexRgb));
    object.insert(QStringLiteral("quickBackground"), theme.quickBackground.name(QColor::HexRgb));
    object.insert(QStringLiteral("quickBorder"), theme.quickBorder.name(QColor::HexRgb));
    object.insert(QStringLiteral("quickHoverBackground"), theme.quickHoverBackground.name(QColor::HexRgb));
    object.insert(QStringLiteral("quickHoverBorder"), theme.quickHoverBorder.name(QColor::HexRgb));
    object.insert(QStringLiteral("tabBackground"), theme.tabBackground.name(QColor::HexRgb));
    object.insert(QStringLiteral("tabHoverBackground"), theme.tabHoverBackground.name(QColor::HexRgb));
    object.insert(QStringLiteral("tabSelectedBackground"), theme.tabSelectedBackground.name(QColor::HexRgb));
    object.insert(QStringLiteral("tabBorder"), theme.tabBorder.name(QColor::HexRgb));
    object.insert(QStringLiteral("tabSelectedBorder"), theme.tabSelectedBorder.name(QColor::HexRgb));
    object.insert(QStringLiteral("tabAccent"), theme.tabAccent.name(QColor::HexRgb));
    object.insert(QStringLiteral("buttonIdleTop"), theme.buttonIdleTop.name(QColor::HexRgb));
    object.insert(QStringLiteral("buttonIdleBottom"), theme.buttonIdleBottom.name(QColor::HexRgb));
    object.insert(QStringLiteral("buttonActiveTop"), theme.buttonActiveTop.name(QColor::HexRgb));
    object.insert(QStringLiteral("buttonActiveBottom"), theme.buttonActiveBottom.name(QColor::HexRgb));
    object.insert(QStringLiteral("buttonPressedTop"), theme.buttonPressedTop.name(QColor::HexRgb));
    object.insert(QStringLiteral("buttonPressedBottom"), theme.buttonPressedBottom.name(QColor::HexRgb));
    object.insert(QStringLiteral("buttonBorder"), theme.buttonBorder.name(QColor::HexRgb));
    object.insert(QStringLiteral("buttonActiveBorder"), theme.buttonActiveBorder.name(QColor::HexRgb));
    object.insert(QStringLiteral("buttonFocusBorder"), theme.buttonFocusBorder.name(QColor::HexRgb));
    object.insert(QStringLiteral("buttonText"), theme.buttonText.name(QColor::HexRgb));
    object.insert(QStringLiteral("buttonPlate"), theme.buttonPlate.name(QColor::HexRgb));
    return object;
}

QJsonObject settingsToJsonObject(const CommandTabSettingsState& state)
{
    QJsonObject object;
    object.insert(QStringLiteral("preferNativeCommandTab"), state.preferNativeCommandTab);
    object.insert(QStringLiteral("nativeCommandTabWarmup"), state.nativeCommandTabWarmup);
    object.insert(QStringLiteral("modernCommandTabStyleEnabled"), state.modernCommandTabStyleEnabled);
    object.insert(QStringLiteral("hideMenuBarInNativeMode"), state.hideMenuBarInNativeMode);
    object.insert(QStringLiteral("nativeThemeMode"), state.nativeThemeMode);
    object.insert(QStringLiteral("compactPanelLayout"), state.compactPanelLayout);
    object.insert(QStringLiteral("compactPanelSpacing"), state.compactPanelSpacing);
    object.insert(QStringLiteral("compactButtonPadding"), state.compactButtonPadding);
    object.insert(QStringLiteral("panelDropdownModeEnabled"), state.panelDropdownModeEnabled);
    object.insert(QStringLiteral("panelDropdownPrimaryRecent"), state.panelDropdownPrimaryRecent);
    object.insert(QStringLiteral("panelDropdownRecentToolCount"), state.panelDropdownRecentToolCount);
    object.insert(QStringLiteral("panelDropdownPopupColumns"), state.panelDropdownPopupColumns);
    object.insert(QStringLiteral("panelDropdownPopupShowText"), state.panelDropdownPopupShowText);
    object.insert(QStringLiteral("panelDropdownPopupIconSize"), state.panelDropdownPopupIconSize);
    object.insert(QStringLiteral("displayScalePercent"), state.displayScalePercent);
    object.insert(QStringLiteral("headerScalePercent"), state.headerScalePercent);
    object.insert(QStringLiteral("commandtabScalePercent"), state.commandtabScalePercent);
    object.insert(QStringLiteral("showIconTextSmall"), state.showIconTextSmall);
    object.insert(QStringLiteral("showIconTextMedium"), state.showIconTextMedium);
    object.insert(QStringLiteral("showIconTextLarge"), state.showIconTextLarge);
    object.insert(QStringLiteral("iconOnlySizeSmall"), state.iconOnlySizeSmall);
    object.insert(QStringLiteral("iconOnlySizeMedium"), state.iconOnlySizeMedium);
    object.insert(QStringLiteral("iconOnlySizeLarge"), state.iconOnlySizeLarge);
    object.insert(QStringLiteral("showSketcherGrid"), state.showSketcherGrid);
    object.insert(QStringLiteral("snapSketcherGrid"), state.snapSketcherGrid);
    object.insert(QStringLiteral("customMainColorsEnabled"), state.customMainColorsEnabled);
    object.insert(QStringLiteral("customMainBackgroundColor"), state.customMainBackgroundColor);
    object.insert(QStringLiteral("customMainTextColor"), state.customMainTextColor);
    object.insert(QStringLiteral("customRibbonPrimaryColor"), state.customRibbonPrimaryColor);
    object.insert(QStringLiteral("customRibbonSecondaryColor"), state.customRibbonSecondaryColor);
    object.insert(QStringLiteral("customRibbonAccentColor"), state.customRibbonAccentColor);
    object.insert(QStringLiteral("ribbonAutoHide"), state.ribbonAutoHide);
    object.insert(QStringLiteral("ribbonAutoHideDelayMs"), state.ribbonAutoHideDelayMs);
    object.insert(QStringLiteral("ribbonHoverTab"), state.ribbonHoverTab);
    object.insert(QStringLiteral("tabClickPopupMode"), state.tabClickPopupMode);
    object.insert(QStringLiteral("viewportColorsEnabled"), state.viewportColorsEnabled);
    object.insert(QStringLiteral("viewportBackgroundStyle"), state.viewportBackgroundStyle);
    object.insert(QStringLiteral("viewportBgTopColor"), state.viewportBgTopColor);
    object.insert(QStringLiteral("viewportBgMidColor"), state.viewportBgMidColor);
    object.insert(QStringLiteral("viewportBgBottomColor"), state.viewportBgBottomColor);
    object.insert(QStringLiteral("viewportBgAccentColor"), state.viewportBgAccentColor);
    object.insert(QStringLiteral("gridColorEnabled"), state.gridColorEnabled);
    object.insert(QStringLiteral("viewportGridColor"), state.viewportGridColor);
    return object;
}

QJsonObject modelToJsonObject(const CommandTabModel& model)
{
    QJsonObject object;
    object.insert(QStringLiteral("activeWorkbenchId"), model.activeWorkbenchId);

    QJsonArray quickAccess;
    for (const auto& command : model.quickAccess) {
        quickAccess.append(commandToJsonObject(command));
    }
    object.insert(QStringLiteral("quickAccess"), quickAccess);

    QJsonArray workbenches;
    for (const auto& workbench : model.workbenches) {
        workbenches.append(workbenchToJsonObject(workbench));
    }
    object.insert(QStringLiteral("workbenches"), workbenches);

    QJsonArray workbenchVisibility;
    for (const auto& entry : model.workbenchVisibility) {
        QJsonObject visibilityObject;
        visibilityObject.insert(QStringLiteral("id"), entry.id);
        visibilityObject.insert(QStringLiteral("title"), entry.title);
        if (!entry.iconPath.isEmpty()) {
            visibilityObject.insert(QStringLiteral("iconPath"), entry.iconPath);
        }
        visibilityObject.insert(QStringLiteral("visible"), entry.visible);
        workbenchVisibility.append(visibilityObject);
    }
    object.insert(QStringLiteral("workbenchVisibility"), workbenchVisibility);

    QJsonArray panelVisibility;
    for (const auto& entry : model.panelVisibility) {
        QJsonObject visibilityObject;
        visibilityObject.insert(QStringLiteral("workbenchId"), entry.workbenchId);
        visibilityObject.insert(QStringLiteral("workbenchTitle"), entry.workbenchTitle);
        visibilityObject.insert(QStringLiteral("id"), entry.id);
        visibilityObject.insert(QStringLiteral("title"), entry.title);
        visibilityObject.insert(QStringLiteral("visible"), entry.visible);
        panelVisibility.append(visibilityObject);
    }
    object.insert(QStringLiteral("panelVisibility"), panelVisibility);

    QJsonArray dropdownDefinitions;
    for (const auto& entry : model.dropdownDefinitions) {
        QJsonObject dropdownObject;
        dropdownObject.insert(QStringLiteral("id"), entry.id);
        dropdownObject.insert(QStringLiteral("text"), entry.text);
        QJsonArray commands;
        for (const auto& command : entry.commands) {
            commands.append(commandToJsonObject(command));
        }
        dropdownObject.insert(QStringLiteral("commands"), commands);
        dropdownDefinitions.append(dropdownObject);
    }
    object.insert(QStringLiteral("dropdownDefinitions"), dropdownDefinitions);
    object.insert(QStringLiteral("settings"), settingsToJsonObject(model.settings));
    object.insert(QStringLiteral("theme"), themeToJsonObject(model.theme));
    return object;
}

CommandTabWorkbenchVisibilityEntry parseWorkbenchVisibility(const QJsonObject& object)
{
    CommandTabWorkbenchVisibilityEntry entry;
    entry.id = object.value(QStringLiteral("id")).toString();
    entry.title = object.value(QStringLiteral("title")).toString(entry.id);
    entry.iconPath = object.value(QStringLiteral("iconPath")).toString();
    entry.visible = object.value(QStringLiteral("visible")).toBool(true);
    return entry;
}

CommandTabPanelVisibilityEntry parsePanelVisibility(const QJsonObject& object)
{
    CommandTabPanelVisibilityEntry entry;
    entry.workbenchId = object.value(QStringLiteral("workbenchId")).toString();
    entry.workbenchTitle = object.value(QStringLiteral("workbenchTitle")).toString(entry.workbenchId);
    entry.id = object.value(QStringLiteral("id")).toString();
    entry.title = object.value(QStringLiteral("title")).toString(entry.id);
    entry.visible = object.value(QStringLiteral("visible")).toBool(true);
    return entry;
}

CommandTabDropdownDefinitionEntry parseDropdownDefinition(const QJsonObject& object)
{
    CommandTabDropdownDefinitionEntry entry;
    entry.id = object.value(QStringLiteral("id")).toString();
    entry.text = object.value(QStringLiteral("text")).toString(entry.id);
    const auto commands = object.value(QStringLiteral("commands")).toArray();
    for (const auto& commandValue : commands) {
        if (!commandValue.isObject()) {
            continue;
        }
        entry.commands.push_back(parseCommand(commandValue.toObject()));
    }
    return entry;
}

bool defaultTextVisibilityForSize(const CommandTabSettingsState& settings, const QString& size)
{
    const QString normalizedSize = size.trimmed().toLower();
    if (normalizedSize == QStringLiteral("medium")) {
        return settings.showIconTextMedium;
    }
    if (normalizedSize == QStringLiteral("large")) {
        return settings.showIconTextLarge;
    }
    return settings.showIconTextSmall;
}

bool resolvedTextVisibilityForCommand(
    const CommandTabSettingsState& settings,
    const QJsonObject& commandObject,
    const QString& size,
    bool* explicitVisibility = nullptr
)
{
    const bool defaultVisibility = defaultTextVisibilityForSize(settings, size);
    bool explicitValue = false;

    if (commandObject.contains(QStringLiteral("textVisibilityExplicit"))) {
        explicitValue = commandObject.value(QStringLiteral("textVisibilityExplicit")).toBool(false);
    } else if (commandObject.contains(QStringLiteral("textVisible"))) {
        explicitValue = true;
    }

    if (explicitVisibility != nullptr) {
        *explicitVisibility = explicitValue;
    }

    if (commandObject.contains(QStringLiteral("textVisible"))) {
        return commandObject.value(QStringLiteral("textVisible")).toBool(defaultVisibility);
    }
    return defaultVisibility;
}

void applySettingsToCommand(CommandTabCommandEntry& command, const CommandTabSettingsState& settings)
{
    if (
        (command.type == QStringLiteral("command") || command.type == QStringLiteral("dropdown"))
        && !command.textVisibilityExplicit
    ) {
        command.textVisible = defaultTextVisibilityForSize(settings, command.size);
    }
    for (auto& menuCommand : command.menuCommands) {
        applySettingsToCommand(menuCommand, settings);
    }
}

void applySettingsToWorkbench(CommandTabWorkbenchEntry& workbench, const CommandTabSettingsState& settings)
{
    for (auto& panel : workbench.panels) {
        for (auto& command : panel.commands) {
            applySettingsToCommand(command, settings);
        }
    }
}

QString correctedCommandDisplayText(const QString& commandId, QString text)
{
    Q_UNUSED(commandId);
    text = text.replace(QStringLiteral("&"), QString()).simplified();
    return text;
}

QString commandtabCommandDisplayText(const CommandTabCommandEntry& command)
{
    if (command.type == QStringLiteral("separator")) {
        return QCoreApplication::translate("CommandTabCustomizationDialog", "Separator");
    }
    const QString normalizedText = command.text.simplified();
    return normalizedText.isEmpty() ? command.id : normalizedText;
}

QJsonObject serializeCustomizationCommand(const CommandTabCommandEntry& command)
{
    QJsonObject object;
    object.insert(QStringLiteral("type"), command.type);
    if (!command.id.isEmpty()) {
        object.insert(QStringLiteral("id"), command.id);
    }
    if (command.type != QStringLiteral("separator")) {
        object.insert(QStringLiteral("text"), commandtabCommandDisplayText(command));
        object.insert(QStringLiteral("size"), command.size.isEmpty() ? QStringLiteral("small") : command.size);
        if (command.textVisibilityExplicit) {
            object.insert(QStringLiteral("textVisible"), command.textVisible);
            object.insert(QStringLiteral("textVisibilityExplicit"), true);
        }
    }
    if (!command.sourceWorkbenchId.isEmpty()) {
        object.insert(QStringLiteral("sourceWorkbenchId"), command.sourceWorkbenchId);
    }
    if (!command.sourceToolbarTitle.isEmpty()) {
        object.insert(QStringLiteral("sourceToolbarTitle"), command.sourceToolbarTitle);
    }
    if (!command.menuCommands.isEmpty()) {
        QJsonArray menuCommands;
        for (const auto& menuCommand : command.menuCommands) {
            menuCommands.append(serializeCustomizationCommand(menuCommand));
        }
        object.insert(QStringLiteral("menuCommands"), menuCommands);
    }
    return object;
}

QByteArray encodeCustomizationPayload(const CommandTabModel& model)
{
    QJsonObject payload;
    QJsonArray quickAccess;
    for (const auto& command : model.quickAccess) {
        if (command.type == QStringLiteral("command") && !command.id.isEmpty()) {
            quickAccess.append(command.id);
        }
    }
    payload.insert(QStringLiteral("quickAccess"), quickAccess);

    QJsonArray hiddenWorkbenches;
    QJsonArray workbenchVisibility;
    for (const auto& workbench : model.workbenchVisibility) {
        QJsonObject workbenchObject;
        workbenchObject.insert(QStringLiteral("id"), workbench.id);
        workbenchObject.insert(QStringLiteral("title"), workbench.title);
        if (!workbench.iconPath.isEmpty()) {
            workbenchObject.insert(QStringLiteral("iconPath"), workbench.iconPath);
        }
        workbenchObject.insert(QStringLiteral("visible"), workbench.visible);
        workbenchVisibility.append(workbenchObject);
        if (!workbench.visible && !workbench.id.isEmpty()) {
            hiddenWorkbenches.append(workbench.id);
        }
    }
    payload.insert(QStringLiteral("workbenchVisibility"), workbenchVisibility);
    payload.insert(QStringLiteral("hiddenWorkbenches"), hiddenWorkbenches);

    QJsonArray hiddenPanels;
    QJsonArray panelVisibility;
    for (const auto& panel : model.panelVisibility) {
        QJsonObject panelObject;
        panelObject.insert(QStringLiteral("workbenchId"), panel.workbenchId);
        panelObject.insert(QStringLiteral("workbenchTitle"), panel.workbenchTitle);
        panelObject.insert(QStringLiteral("id"), panel.id);
        panelObject.insert(QStringLiteral("title"), panel.title);
        panelObject.insert(QStringLiteral("visible"), panel.visible);
        panelVisibility.append(panelObject);
        if (!panel.visible && !panel.id.isEmpty()) {
            hiddenPanels.append(panel.id);
        }
    }
    payload.insert(QStringLiteral("panelVisibility"), panelVisibility);
    payload.insert(QStringLiteral("hiddenPanels"), hiddenPanels);

    QJsonArray dropdowns;
    for (const auto& dropdown : model.dropdownDefinitions) {
        if (dropdown.id.isEmpty()) {
            continue;
        }
        QJsonObject dropdownObject;
        dropdownObject.insert(QStringLiteral("id"), dropdown.id);
        dropdownObject.insert(QStringLiteral("text"), dropdown.text);
        QJsonArray commands;
        for (const auto& command : dropdown.commands) {
            commands.append(serializeCustomizationCommand(command));
        }
        dropdownObject.insert(QStringLiteral("commands"), commands);
        dropdowns.append(dropdownObject);
    }
    payload.insert(QStringLiteral("dropdowns"), dropdowns);

    QJsonArray workbenches;
    for (const auto& workbench : model.workbenches) {
        if (workbench.id.isEmpty() || workbench.panels.isEmpty()) {
            continue;
        }

        QJsonObject workbenchObject;
        workbenchObject.insert(QStringLiteral("id"), workbench.id);
        QJsonArray panels;
        for (const auto& panel : workbench.panels) {
            if (panel.id.isEmpty()) {
                continue;
            }

            QJsonObject panelObject;
            panelObject.insert(QStringLiteral("id"), panel.id);
            panelObject.insert(QStringLiteral("title"), panel.title);
            if (!panel.sourceType.isEmpty()) {
                panelObject.insert(QStringLiteral("sourceType"), panel.sourceType);
            }
            if (!panel.sourceWorkbenchId.isEmpty()) {
                panelObject.insert(QStringLiteral("sourceWorkbenchId"), panel.sourceWorkbenchId);
            }
            QJsonArray commands;
            for (const auto& command : panel.commands) {
                commands.append(serializeCustomizationCommand(command));
            }
            panelObject.insert(QStringLiteral("commands"), commands);
            panels.append(panelObject);
        }
        workbenchObject.insert(QStringLiteral("panels"), panels);
        workbenches.append(workbenchObject);
    }
    payload.insert(QStringLiteral("workbenches"), workbenches);

    const QByteArray json = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    return json.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

QString encodeCommandTabCommandArgument(const QString& value)
{
    return QString::fromLatin1(
        value.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)
    );
}

CommandTabSettingsState parseSettingsState(const QJsonObject& object)
{
    CommandTabSettingsState state;
    auto parseClampedIconSize = [&object](const QString& key, int fallback) -> int {
        const int value = object.value(key).toInt(fallback);
        return std::clamp(value, 12, 64);
    };
    auto parseClampedCompactInt = [&object](const QString& key, int fallback, int minimum, int maximum) -> int {
        const int value = object.value(key).toInt(fallback);
        return std::clamp(value, minimum, maximum);
    };
    state.preferNativeCommandTab = object.value(QStringLiteral("preferNativeCommandTab")).toBool(state.preferNativeCommandTab);
    state.nativeCommandTabWarmup = object.value(QStringLiteral("nativeCommandTabWarmup")).toBool(state.nativeCommandTabWarmup);
    state.modernCommandTabStyleEnabled = object.value(QStringLiteral("modernCommandTabStyleEnabled")).toBool(state.modernCommandTabStyleEnabled);
    state.hideMenuBarInNativeMode =
        object.value(QStringLiteral("hideMenuBarInNativeMode"))
            .toBool(state.hideMenuBarInNativeMode);
    state.compactPanelLayout = object.value(QStringLiteral("compactPanelLayout")).toBool(state.compactPanelLayout);
    state.compactPanelSpacing = parseClampedCompactInt(
        QStringLiteral("compactPanelSpacing"),
        state.compactPanelSpacing,
        0,
        6
    );
    state.compactButtonPadding = parseClampedCompactInt(
        QStringLiteral("compactButtonPadding"),
        state.compactButtonPadding,
        1,
        8
    );
    state.panelDropdownModeEnabled =
        object.value(QStringLiteral("panelDropdownModeEnabled"))
            .toBool(state.panelDropdownModeEnabled);
    state.panelDropdownPrimaryRecent =
        object.value(QStringLiteral("panelDropdownPrimaryRecent"))
            .toBool(state.panelDropdownPrimaryRecent);
    state.panelDropdownRecentToolCount = parseClampedCompactInt(
        QStringLiteral("panelDropdownRecentToolCount"),
        state.panelDropdownRecentToolCount,
        0,
        9
    );
    state.panelDropdownPopupColumns = parseClampedCompactInt(
        QStringLiteral("panelDropdownPopupColumns"),
        state.panelDropdownPopupColumns,
        2,
        8
    );
    state.panelDropdownPopupShowText =
        object.value(QStringLiteral("panelDropdownPopupShowText"))
            .toBool(state.panelDropdownPopupShowText);
    state.panelDropdownPopupIconSize = parseClampedCompactInt(
        QStringLiteral("panelDropdownPopupIconSize"),
        state.panelDropdownPopupIconSize,
        12,
        48
    );
    state.displayScalePercent = parseClampedCompactInt(
        QStringLiteral("displayScalePercent"),
        state.displayScalePercent,
        60,
        140
    );
    state.headerScalePercent = parseClampedCompactInt(
        QStringLiteral("headerScalePercent"),
        state.displayScalePercent,
        60,
        140
    );
    state.commandtabScalePercent = parseClampedCompactInt(
        QStringLiteral("commandtabScalePercent"),
        state.displayScalePercent,
        60,
        140
    );
    state.showIconTextSmall = object.value(QStringLiteral("showIconTextSmall")).toBool(state.showIconTextSmall);
    state.showIconTextMedium = object.value(QStringLiteral("showIconTextMedium")).toBool(state.showIconTextMedium);
    state.showIconTextLarge = object.value(QStringLiteral("showIconTextLarge")).toBool(state.showIconTextLarge);
    state.iconOnlySizeSmall = parseClampedIconSize(QStringLiteral("iconOnlySizeSmall"), state.iconOnlySizeSmall);
    state.iconOnlySizeMedium = parseClampedIconSize(QStringLiteral("iconOnlySizeMedium"), state.iconOnlySizeMedium);
    state.iconOnlySizeLarge = parseClampedIconSize(QStringLiteral("iconOnlySizeLarge"), state.iconOnlySizeLarge);
    state.showSketcherGrid = object.value(QStringLiteral("showSketcherGrid")).toBool(state.showSketcherGrid);
    state.snapSketcherGrid = object.value(QStringLiteral("snapSketcherGrid")).toBool(state.snapSketcherGrid);
    state.customMainColorsEnabled =
        object.value(QStringLiteral("customMainColorsEnabled"))
            .toBool(state.customMainColorsEnabled);
    state.customMainBackgroundColor = normalizedColorHex(
        object.value(QStringLiteral("customMainBackgroundColor"))
            .toString(state.customMainBackgroundColor)
    );
    state.customMainTextColor = normalizedColorHex(
        object.value(QStringLiteral("customMainTextColor"))
            .toString(state.customMainTextColor)
    );
    state.customRibbonPrimaryColor = normalizedColorHex(
        object.value(QStringLiteral("customRibbonPrimaryColor"))
            .toString(state.customRibbonPrimaryColor)
    );
    state.customRibbonSecondaryColor = normalizedColorHex(
        object.value(QStringLiteral("customRibbonSecondaryColor"))
            .toString(state.customRibbonSecondaryColor)
    );
    state.customRibbonAccentColor = normalizedColorHex(
        object.value(QStringLiteral("customRibbonAccentColor"))
            .toString(state.customRibbonAccentColor)
    );

    const QString nativeThemeMode = object.value(QStringLiteral("nativeThemeMode")).toString(state.nativeThemeMode).trimmed().toLower();
    if (nativeThemeMode == QStringLiteral("dark") || nativeThemeMode == QStringLiteral("light")) {
        state.nativeThemeMode = nativeThemeMode;
    } else {
        state.nativeThemeMode = QStringLiteral("auto");
    }
    state.ribbonAutoHide = object.value(QStringLiteral("ribbonAutoHide")).toBool(false);
    state.ribbonAutoHideDelayMs = object.value(QStringLiteral("ribbonAutoHideDelayMs")).toInt(1500);
    state.ribbonHoverTab = object.value(QStringLiteral("ribbonHoverTab")).toBool(false);
    state.tabClickPopupMode = object.value(QStringLiteral("tabClickPopupMode")).toBool(false);
    state.viewportColorsEnabled = object.value(QStringLiteral("viewportColorsEnabled")).toBool(false);
    state.viewportBackgroundStyle =
        object.value(QStringLiteral("viewportBackgroundStyle"))
            .toString(state.viewportBackgroundStyle)
            .trimmed()
            .toLower();
    if (
        state.viewportBackgroundStyle != QStringLiteral("solid")
        && state.viewportBackgroundStyle != QStringLiteral("linear")
        && state.viewportBackgroundStyle != QStringLiteral("tricolor")
        && state.viewportBackgroundStyle != QStringLiteral("radial")
        && state.viewportBackgroundStyle != QStringLiteral("quad")
    ) {
        state.viewportBackgroundStyle = QStringLiteral("linear");
    }
    state.viewportBgTopColor = object.value(QStringLiteral("viewportBgTopColor")).toString();
    state.viewportBgMidColor = object.value(QStringLiteral("viewportBgMidColor")).toString();
    state.viewportBgBottomColor = object.value(QStringLiteral("viewportBgBottomColor")).toString();
    state.viewportBgAccentColor = object.value(QStringLiteral("viewportBgAccentColor")).toString();
    state.gridColorEnabled = object.value(QStringLiteral("gridColorEnabled")).toBool(false);
    state.viewportGridColor = object.value(QStringLiteral("viewportGridColor")).toString();

    return state;
}
