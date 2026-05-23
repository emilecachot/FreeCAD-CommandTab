class CommandTabSettingsDialog final : public QDialog
{
public:
    explicit CommandTabSettingsDialog(QWidget* parent = nullptr)
        : QDialog(parent)
    {
        auto trLabel = [](const char* text) {
            return QCoreApplication::translate("CommandTabSettingsDialog", text);
        };

        setObjectName(QStringLiteral("CommandTabSettingsDialog"));
        setWindowTitle(trLabel("CommandTab preferences"));
        setModal(false);
        resize(620, 520);

        auto* rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(12, 12, 12, 12);
        rootLayout->setSpacing(10);

        auto* headerLabel = new QLabel(trLabel("Native commandtab preferences"), this);
        headerLabel->setObjectName(QStringLiteral("CommandTabSettingsHeader"));
        rootLayout->addWidget(headerLabel);

        auto* generalTab = new QWidget(this);
        auto* generalLayout = new QGridLayout(generalTab);
        generalLayout->setContentsMargins(0, 0, 0, 0);
        generalLayout->setHorizontalSpacing(10);
        generalLayout->setVerticalSpacing(8);

        int generalRow = 0;
        m_preferNativeCommandTabCheck = new QCheckBox(trLabel("Prefer native commandtab"), generalTab);
        generalLayout->addWidget(m_preferNativeCommandTabCheck, generalRow++, 0, 1, 2);

        m_nativeCommandTabWarmupCheck =
            new QCheckBox(trLabel("Warm up workbenches in background"), generalTab);
        generalLayout->addWidget(m_nativeCommandTabWarmupCheck, generalRow++, 0, 1, 2);

        m_modernCommandTabStyleCheck =
            new QCheckBox(trLabel("Enable modern commandtab style"), generalTab);
        generalLayout->addWidget(m_modernCommandTabStyleCheck, generalRow++, 0, 1, 2);

        m_hideMenuBarInNativeModeCheck =
            new QCheckBox(
                QCoreApplication::translate(
                    "CommandTabSettingsDialog",
                    "Hide FreeCAD menu bar in native mode"
                ),
                generalTab
            );
        generalLayout->addWidget(m_hideMenuBarInNativeModeCheck, generalRow++, 0, 1, 2);

        m_ribbonAutoHideCheck = new QCheckBox(trLabel("Auto-hide ribbon (collapse to tab bar)"), generalTab);
        m_ribbonAutoHideCheck->setToolTip(
            trLabel(
                "Collapse the ribbon to just the tab bar.\n"
                "Click a tab to temporarily expand it.\n"
                "Use the \342\226\262/\342\226\274 button to pin or close it manually."
            )
        );
        generalLayout->addWidget(m_ribbonAutoHideCheck, generalRow++, 0, 1, 2);

        generalLayout->addWidget(new QLabel(trLabel("Auto-hide delay"), generalTab), generalRow, 0);
        m_ribbonAutoHideDelaySpinner = new QSpinBox(generalTab);
        m_ribbonAutoHideDelaySpinner->setRange(300, 10000);
        m_ribbonAutoHideDelaySpinner->setSingleStep(250);
        m_ribbonAutoHideDelaySpinner->setSuffix(QStringLiteral(" ms"));
        m_ribbonAutoHideDelaySpinner->setToolTip(
            trLabel("Time before the ribbon hides after the mouse leaves it or a command is run.")
        );
        generalLayout->addWidget(m_ribbonAutoHideDelaySpinner, generalRow++, 1);

        m_ribbonHoverTabCheck = new QCheckBox(trLabel("Switch tab on hover"), generalTab);
        m_ribbonHoverTabCheck->setToolTip(
            trLabel("Hovering over a tab for a moment switches to it without clicking.")
        );
        generalLayout->addWidget(m_ribbonHoverTabCheck, generalRow++, 0, 1, 2);

        m_tabClickPopupModeCheck = new QCheckBox(
            trLabel("Show tab tools as popup bubble"),
            generalTab
        );
        m_tabClickPopupModeCheck->setToolTip(
            trLabel(
                "When enabled, clicking a tab opens a popup of that workbench tools\n"
                "without fully expanding the ribbon panels."
            )
        );
        generalLayout->addWidget(m_tabClickPopupModeCheck, generalRow++, 0, 1, 2);

        generalLayout->addWidget(new QLabel(trLabel("Native theme"), generalTab), generalRow, 0);
        m_nativeThemeModeCombo = new QComboBox(generalTab);
        m_nativeThemeModeCombo->setIconSize(QSize(44, 22));
        m_nativeThemeModeCombo->addItem(trLabel("Auto"), QStringLiteral("auto"));
        m_nativeThemeModeCombo->addItem(trLabel("Dark"), QStringLiteral("dark"));
        m_nativeThemeModeCombo->addItem(trLabel("Light"), QStringLiteral("light"));
        generalLayout->addWidget(m_nativeThemeModeCombo, generalRow++, 1);
        m_nativeThemePreviewLabel = new QLabel(generalTab);
        m_nativeThemePreviewLabel->setObjectName(QStringLiteral("CommandTabNativeThemePreview"));
        m_nativeThemePreviewLabel->setAlignment(Qt::AlignCenter);
        m_nativeThemePreviewLabel->setFrameShape(QFrame::NoFrame);
        m_nativeThemePreviewLabel->setMinimumHeight(58);
        generalLayout->addWidget(m_nativeThemePreviewLabel, generalRow++, 0, 1, 2);

        generalLayout->addWidget(new QLabel(trLabel("Top bar scale"), generalTab), generalRow, 0);
        m_headerScalePercentSpin = new QSpinBox(generalTab);
        m_headerScalePercentSpin->setRange(60, 140);
        m_headerScalePercentSpin->setSingleStep(5);
        m_headerScalePercentSpin->setSuffix(QStringLiteral(" %"));
        m_headerScalePercentSpin->setToolTip(
            trLabel("Scale the top bar (tabs, quick access, utility buttons)")
        );
        generalLayout->addWidget(m_headerScalePercentSpin, generalRow++, 1);

        generalLayout->addWidget(new QLabel(trLabel("CommandTab scale"), generalTab), generalRow, 0);
        m_commandtabScalePercentSpin = new QSpinBox(generalTab);
        m_commandtabScalePercentSpin->setRange(60, 140);
        m_commandtabScalePercentSpin->setSingleStep(5);
        m_commandtabScalePercentSpin->setSuffix(QStringLiteral(" %"));
        m_commandtabScalePercentSpin->setToolTip(
            trLabel("Scale commandtab panels, tools and popup content")
        );
        generalLayout->addWidget(m_commandtabScalePercentSpin, generalRow++, 1);
        generalLayout->setRowStretch(generalRow, 1);

        auto* commandtabLayoutTab = new QWidget(this);
        auto* commandtabLayoutGrid = new QGridLayout(commandtabLayoutTab);
        commandtabLayoutGrid->setContentsMargins(0, 0, 0, 0);
        commandtabLayoutGrid->setHorizontalSpacing(10);
        commandtabLayoutGrid->setVerticalSpacing(8);

        int layoutRow = 0;
        m_compactPanelLayoutCheck = new QCheckBox(trLabel("Use compact panel layout"), commandtabLayoutTab);
        commandtabLayoutGrid->addWidget(m_compactPanelLayoutCheck, layoutRow++, 0, 1, 2);

        commandtabLayoutGrid->addWidget(new QLabel(trLabel("Panel spacing"), commandtabLayoutTab), layoutRow, 0);
        m_compactPanelSpacingSpin = new QSpinBox(commandtabLayoutTab);
        m_compactPanelSpacingSpin->setRange(0, 6);
        m_compactPanelSpacingSpin->setSingleStep(1);
        m_compactPanelSpacingSpin->setSuffix(QStringLiteral(" px"));
        commandtabLayoutGrid->addWidget(m_compactPanelSpacingSpin, layoutRow++, 1);

        commandtabLayoutGrid->addWidget(new QLabel(trLabel("Button padding"), commandtabLayoutTab), layoutRow, 0);
        m_compactButtonPaddingSpin = new QSpinBox(commandtabLayoutTab);
        m_compactButtonPaddingSpin->setRange(1, 8);
        m_compactButtonPaddingSpin->setSingleStep(1);
        m_compactButtonPaddingSpin->setSuffix(QStringLiteral(" px"));
        commandtabLayoutGrid->addWidget(m_compactButtonPaddingSpin, layoutRow++, 1);

        m_panelDropdownModeCheck = new QCheckBox(
            trLabel("Enable panel dropdown arrows"),
            commandtabLayoutTab
        );
        commandtabLayoutGrid->addWidget(m_panelDropdownModeCheck, layoutRow++, 0, 1, 2);

        m_panelDropdownPrimaryRecentCheck = new QCheckBox(
            trLabel("Use last used tool as panel primary"),
            commandtabLayoutTab
        );
        commandtabLayoutGrid->addWidget(m_panelDropdownPrimaryRecentCheck, layoutRow++, 0, 1, 2);
        m_panelDropdownPrimaryRecentCheck->setEnabled(false);

        auto* panelDropdownHeader = new QLabel(trLabel("Panel dropdown behavior"), commandtabLayoutTab);
        commandtabLayoutGrid->addWidget(panelDropdownHeader, layoutRow++, 0, 1, 2);

        commandtabLayoutGrid->addWidget(new QLabel(trLabel("Recent side tools"), commandtabLayoutTab), layoutRow, 0);
        m_panelDropdownRecentToolCountSpin = new QSpinBox(commandtabLayoutTab);
        m_panelDropdownRecentToolCountSpin->setRange(0, 9);
        m_panelDropdownRecentToolCountSpin->setSingleStep(1);
        m_panelDropdownRecentToolCountSpin->setToolTip(
            trLabel("How many recent tools appear next to the primary split button")
        );
        commandtabLayoutGrid->addWidget(m_panelDropdownRecentToolCountSpin, layoutRow++, 1);

        commandtabLayoutGrid->addWidget(new QLabel(trLabel("Popup grid columns"), commandtabLayoutTab), layoutRow, 0);
        m_panelDropdownPopupColumnsSpin = new QSpinBox(commandtabLayoutTab);
        m_panelDropdownPopupColumnsSpin->setRange(2, 8);
        m_panelDropdownPopupColumnsSpin->setSingleStep(1);
        commandtabLayoutGrid->addWidget(m_panelDropdownPopupColumnsSpin, layoutRow++, 1);

        commandtabLayoutGrid->addWidget(new QLabel(trLabel("Popup icon size"), commandtabLayoutTab), layoutRow, 0);
        m_panelDropdownPopupIconSizeSpin = new QSpinBox(commandtabLayoutTab);
        m_panelDropdownPopupIconSizeSpin->setRange(12, 48);
        m_panelDropdownPopupIconSizeSpin->setSingleStep(1);
        m_panelDropdownPopupIconSizeSpin->setSuffix(QStringLiteral(" px"));
        commandtabLayoutGrid->addWidget(m_panelDropdownPopupIconSizeSpin, layoutRow++, 1);

        m_panelDropdownPopupShowTextCheck = new QCheckBox(
            trLabel("Show text in popup grid"),
            commandtabLayoutTab
        );
        commandtabLayoutGrid->addWidget(m_panelDropdownPopupShowTextCheck, layoutRow++, 0, 1, 2);
        commandtabLayoutGrid->setRowStretch(layoutRow, 1);

        auto* toolsTab = new QWidget(this);
        auto* toolsLayout = new QGridLayout(toolsTab);
        toolsLayout->setContentsMargins(0, 0, 0, 0);
        toolsLayout->setHorizontalSpacing(10);
        toolsLayout->setVerticalSpacing(8);

        int toolsRow = 0;
        m_showIconTextSmallCheck = new QCheckBox(trLabel("Show text on small tools"), toolsTab);
        toolsLayout->addWidget(m_showIconTextSmallCheck, toolsRow++, 0, 1, 2);

        m_showIconTextMediumCheck = new QCheckBox(trLabel("Show text on medium tools"), toolsTab);
        toolsLayout->addWidget(m_showIconTextMediumCheck, toolsRow++, 0, 1, 2);

        m_showIconTextLargeCheck = new QCheckBox(trLabel("Show text on large tools"), toolsTab);
        toolsLayout->addWidget(m_showIconTextLargeCheck, toolsRow++, 0, 1, 2);

        auto* iconOnlySizeHeader = new QLabel(trLabel("Icon size without text (px)"), toolsTab);
        toolsLayout->addWidget(iconOnlySizeHeader, toolsRow++, 0, 1, 2);

        toolsLayout->addWidget(new QLabel(trLabel("Small"), toolsTab), toolsRow, 0);
        m_iconOnlySizeSmallSpin = new QSpinBox(toolsTab);
        m_iconOnlySizeSmallSpin->setRange(12, 64);
        m_iconOnlySizeSmallSpin->setSingleStep(1);
        m_iconOnlySizeSmallSpin->setSuffix(QStringLiteral(" px"));
        toolsLayout->addWidget(m_iconOnlySizeSmallSpin, toolsRow++, 1);

        toolsLayout->addWidget(new QLabel(trLabel("Medium"), toolsTab), toolsRow, 0);
        m_iconOnlySizeMediumSpin = new QSpinBox(toolsTab);
        m_iconOnlySizeMediumSpin->setRange(12, 64);
        m_iconOnlySizeMediumSpin->setSingleStep(1);
        m_iconOnlySizeMediumSpin->setSuffix(QStringLiteral(" px"));
        toolsLayout->addWidget(m_iconOnlySizeMediumSpin, toolsRow++, 1);

        toolsLayout->addWidget(new QLabel(trLabel("Large"), toolsTab), toolsRow, 0);
        m_iconOnlySizeLargeSpin = new QSpinBox(toolsTab);
        m_iconOnlySizeLargeSpin->setRange(12, 64);
        m_iconOnlySizeLargeSpin->setSingleStep(1);
        m_iconOnlySizeLargeSpin->setSuffix(QStringLiteral(" px"));
        toolsLayout->addWidget(m_iconOnlySizeLargeSpin, toolsRow++, 1);
        toolsLayout->setRowStretch(toolsRow, 1);

        auto* sketcherTab = new QWidget(this);
        auto* sketcherLayout = new QVBoxLayout(sketcherTab);
        sketcherLayout->setContentsMargins(0, 0, 0, 0);
        sketcherLayout->setSpacing(8);
        m_showSketcherGridCheck = new QCheckBox(trLabel("Keep sketcher grid visible"), sketcherTab);
        sketcherLayout->addWidget(m_showSketcherGridCheck);

        m_snapSketcherGridCheck = new QCheckBox(trLabel("Keep sketcher snap enabled"), sketcherTab);
        sketcherLayout->addWidget(m_snapSketcherGridCheck);
        sketcherLayout->addStretch(1);

        auto* profileTab = new QWidget(this);
        auto* profileLayout = new QVBoxLayout(profileTab);
        profileLayout->setContentsMargins(0, 0, 0, 0);
        profileLayout->setSpacing(10);

        auto* profileHint = new QLabel(
            trLabel(
                "Export the current commandtab layout to a JSON file, or import a layout "
                "from a previously exported file. Resetting restores the built-in defaults."
            ),
            profileTab
        );
        profileHint->setWordWrap(true);
        profileLayout->addWidget(profileHint);

        auto* exportProfileButton = new QPushButton(trLabel("Export profile…"), profileTab);
        exportProfileButton->setObjectName(QStringLiteral("CommandTabExportProfileButton"));
        profileLayout->addWidget(exportProfileButton);

        auto* importProfileButton = new QPushButton(trLabel("Import profile…"), profileTab);
        importProfileButton->setObjectName(QStringLiteral("CommandTabImportProfileButton"));
        profileLayout->addWidget(importProfileButton);

        auto* resetProfileButton = new QPushButton(trLabel("Reset to defaults"), profileTab);
        resetProfileButton->setObjectName(QStringLiteral("CommandTabResetProfileButton"));
        profileLayout->addWidget(resetProfileButton);

        profileLayout->addStretch(1);

        connect(exportProfileButton, &QPushButton::clicked, this, [this]() {
            exportProfile();
        });
        connect(importProfileButton, &QPushButton::clicked, this, [this]() {
            importProfile();
        });
        connect(resetProfileButton, &QPushButton::clicked, this, [this]() {
            if (!m_commandHandler) {
                return;
            }
            m_commandHandler(QStringLiteral("__commandtab_design_reset__"));
        });

        auto setPanelDropdownControlsEnabled = [this](bool enabled) {
            m_panelDropdownPrimaryRecentCheck->setEnabled(enabled);
            m_panelDropdownRecentToolCountSpin->setEnabled(enabled);
            m_panelDropdownPopupColumnsSpin->setEnabled(enabled);
            m_panelDropdownPopupIconSizeSpin->setEnabled(enabled);
            m_panelDropdownPopupShowTextCheck->setEnabled(enabled);
        };
        connect(
            m_panelDropdownModeCheck,
            &QCheckBox::toggled,
            this,
            [setPanelDropdownControlsEnabled](bool enabled) {
                setPanelDropdownControlsEnabled(enabled);
            }
        );
        setPanelDropdownControlsEnabled(false);

        auto* colorsTab = new QWidget(this);
        auto* colorsLayout = new QGridLayout(colorsTab);
        colorsLayout->setContentsMargins(0, 0, 0, 0);
        colorsLayout->setHorizontalSpacing(10);
        colorsLayout->setVerticalSpacing(8);

        int colorsRow = 0;
        m_customMainColorsEnabledCheck = new QCheckBox(
            trLabel("Override main commandtab colors"),
            colorsTab
        );
        colorsLayout->addWidget(m_customMainColorsEnabledCheck, colorsRow++, 0, 1, 2);

        auto* colorsHint = new QLabel(
            trLabel(
                "Define a clean ribbon palette: primary surface, secondary surface, accent, "
                "plus optional text override."
            ),
            colorsTab
        );
        colorsHint->setWordWrap(true);
        colorsLayout->addWidget(colorsHint, colorsRow++, 0, 1, 2);

        m_colorPreviewFrame = new QFrame(colorsTab);
        m_colorPreviewFrame->setObjectName(QStringLiteral("CommandTabColorPreviewFrame"));
        m_colorPreviewFrame->setFrameShape(QFrame::NoFrame);
        m_colorPreviewFrame->setMaximumHeight(188);
        auto* colorPreviewLayout = new QVBoxLayout(m_colorPreviewFrame);
        colorPreviewLayout->setContentsMargins(10, 8, 10, 10);
        colorPreviewLayout->setSpacing(6);

        auto* colorPreviewHeader = new QLabel(trLabel("Live ribbon preview"), m_colorPreviewFrame);
        colorPreviewHeader->setObjectName(QStringLiteral("CommandTabColorPreviewHeader"));
        colorPreviewLayout->addWidget(colorPreviewHeader);

        m_colorPreviewRibbonRow = new QWidget(m_colorPreviewFrame);
        m_colorPreviewRibbonRow->setObjectName(QStringLiteral("CommandTabColorPreviewRibbonRow"));
        auto* previewRibbonLayout = new QHBoxLayout(m_colorPreviewRibbonRow);
        previewRibbonLayout->setContentsMargins(6, 5, 6, 5);
        previewRibbonLayout->setSpacing(6);

        m_colorPreviewLogoLeft = new QLabel(m_colorPreviewRibbonRow);
        m_colorPreviewLogoLeft->setObjectName(QStringLiteral("CommandTabColorPreviewLogoLeft"));
        m_colorPreviewLogoLeft->setFixedSize(20, 20);
        m_colorPreviewLogoLeft->setAlignment(Qt::AlignCenter);
        m_colorPreviewLogoLeft->setFrameStyle(QFrame::NoFrame);
        previewRibbonLayout->addWidget(m_colorPreviewLogoLeft, 0, Qt::AlignVCenter);

        m_colorPreviewTabActive = new QLabel(trLabel("Atelier"), m_colorPreviewRibbonRow);
        m_colorPreviewTabActive->setObjectName(QStringLiteral("CommandTabColorPreviewTabActive"));
        m_colorPreviewTabActive->setAlignment(Qt::AlignCenter);
        m_colorPreviewTabActive->setFrameStyle(QFrame::NoFrame);
        previewRibbonLayout->addWidget(m_colorPreviewTabActive, 0, Qt::AlignVCenter);

        m_colorPreviewTabInactive = new QLabel(trLabel("Esquisse"), m_colorPreviewRibbonRow);
        m_colorPreviewTabInactive->setObjectName(QStringLiteral("CommandTabColorPreviewTabInactive"));
        m_colorPreviewTabInactive->setAlignment(Qt::AlignCenter);
        m_colorPreviewTabInactive->setFrameStyle(QFrame::NoFrame);
        previewRibbonLayout->addWidget(m_colorPreviewTabInactive, 0, Qt::AlignVCenter);

        previewRibbonLayout->addStretch(1);

        m_colorPreviewAccentBadge = new QLabel(trLabel("Accent"), m_colorPreviewRibbonRow);
        m_colorPreviewAccentBadge->setObjectName(QStringLiteral("CommandTabColorPreviewAccentBadge"));
        m_colorPreviewAccentBadge->setAlignment(Qt::AlignCenter);
        m_colorPreviewAccentBadge->setFrameStyle(QFrame::NoFrame);
        previewRibbonLayout->addWidget(m_colorPreviewAccentBadge, 0, Qt::AlignVCenter);

        m_colorPreviewLogoRight = new QLabel(m_colorPreviewRibbonRow);
        m_colorPreviewLogoRight->setObjectName(QStringLiteral("CommandTabColorPreviewLogoRight"));
        m_colorPreviewLogoRight->setFixedSize(20, 20);
        m_colorPreviewLogoRight->setAlignment(Qt::AlignCenter);
        m_colorPreviewLogoRight->setFrameStyle(QFrame::NoFrame);
        previewRibbonLayout->addWidget(m_colorPreviewLogoRight, 0, Qt::AlignVCenter);

        colorPreviewLayout->addWidget(m_colorPreviewRibbonRow);

        m_colorPreviewPanelCard = new QFrame(m_colorPreviewFrame);
        m_colorPreviewPanelCard->setObjectName(QStringLiteral("CommandTabColorPreviewPanelCard"));
        m_colorPreviewPanelCard->setFrameShape(QFrame::NoFrame);
        auto* previewPanelLayout = new QVBoxLayout(m_colorPreviewPanelCard);
        previewPanelLayout->setContentsMargins(8, 8, 8, 8);
        previewPanelLayout->setSpacing(6);

        m_colorPreviewPanelTitle = new QLabel(trLabel("Boîte à outils"), m_colorPreviewPanelCard);
        m_colorPreviewPanelTitle->setObjectName(QStringLiteral("CommandTabColorPreviewPanelTitle"));
        m_colorPreviewPanelTitle->setAlignment(Qt::AlignCenter);
        m_colorPreviewPanelTitle->setFrameStyle(QFrame::NoFrame);
        previewPanelLayout->addWidget(m_colorPreviewPanelTitle);

        m_colorPreviewPanelBody = new QLabel(trLabel("Texte, onglets, badges et panneaux"), m_colorPreviewPanelCard);
        m_colorPreviewPanelBody->setObjectName(QStringLiteral("CommandTabColorPreviewPanelBody"));
        m_colorPreviewPanelBody->setWordWrap(true);
        m_colorPreviewPanelBody->setFrameStyle(QFrame::NoFrame);
        previewPanelLayout->addWidget(m_colorPreviewPanelBody);

        colorPreviewLayout->addWidget(m_colorPreviewPanelCard);
        colorsLayout->addWidget(m_colorPreviewFrame, colorsRow++, 0, 1, 2);

        colorsLayout->addWidget(new QLabel(trLabel("Main background"), colorsTab), colorsRow, 0);
        m_customMainBackgroundButton = new QPushButton(colorsTab);
        m_customMainBackgroundButton->setObjectName(QStringLiteral("CommandTabColorPickButtonBackground"));
        m_customMainBackgroundButton->setMinimumWidth(170);
        colorsLayout->addWidget(m_customMainBackgroundButton, colorsRow++, 1);

        colorsLayout->addWidget(new QLabel(trLabel("Main text"), colorsTab), colorsRow, 0);
        m_customMainTextButton = new QPushButton(colorsTab);
        m_customMainTextButton->setObjectName(QStringLiteral("CommandTabColorPickButtonText"));
        m_customMainTextButton->setMinimumWidth(170);
        colorsLayout->addWidget(m_customMainTextButton, colorsRow++, 1);

        colorsLayout->addWidget(new QLabel(trLabel("Ribbon primary"), colorsTab), colorsRow, 0);
        m_customRibbonPrimaryButton = new QPushButton(colorsTab);
        m_customRibbonPrimaryButton->setObjectName(QStringLiteral("CommandTabColorPickButtonRibbonPrimary"));
        m_customRibbonPrimaryButton->setMinimumWidth(170);
        colorsLayout->addWidget(m_customRibbonPrimaryButton, colorsRow++, 1);

        colorsLayout->addWidget(new QLabel(trLabel("Ribbon secondary"), colorsTab), colorsRow, 0);
        m_customRibbonSecondaryButton = new QPushButton(colorsTab);
        m_customRibbonSecondaryButton->setObjectName(QStringLiteral("CommandTabColorPickButtonRibbonSecondary"));
        m_customRibbonSecondaryButton->setMinimumWidth(170);
        colorsLayout->addWidget(m_customRibbonSecondaryButton, colorsRow++, 1);

        colorsLayout->addWidget(new QLabel(trLabel("Ribbon accent"), colorsTab), colorsRow, 0);
        m_customRibbonAccentButton = new QPushButton(colorsTab);
        m_customRibbonAccentButton->setObjectName(QStringLiteral("CommandTabColorPickButtonRibbonAccent"));
        m_customRibbonAccentButton->setMinimumWidth(170);
        colorsLayout->addWidget(m_customRibbonAccentButton, colorsRow++, 1);

        auto* resetColorsButton = new QPushButton(trLabel("Reset to theme defaults"), colorsTab);
        colorsLayout->addWidget(resetColorsButton, colorsRow++, 0, 1, 2);

        // ── Viewport background ────────────────────────��────────────────
        auto* viewportSep = new QFrame(colorsTab);
        viewportSep->setFrameShape(QFrame::HLine);
        viewportSep->setFrameShadow(QFrame::Sunken);
        colorsLayout->addWidget(viewportSep, colorsRow++, 0, 1, 2);

        m_viewportColorsEnabledCheck = new QCheckBox(trLabel("Override viewport background"), colorsTab);
        m_viewportColorsEnabledCheck->setToolTip(
            trLabel("Set a custom background color for the 3D view.")
        );
        colorsLayout->addWidget(m_viewportColorsEnabledCheck, colorsRow++, 0, 1, 2);

        colorsLayout->addWidget(new QLabel(trLabel("Background style"), colorsTab), colorsRow, 0);
        m_viewportBackgroundStyleCombo = new QComboBox(colorsTab);
        m_viewportBackgroundStyleCombo->setIconSize(QSize(44, 22));
        m_viewportBackgroundStyleCombo->addItem(trLabel("Solid"), QStringLiteral("solid"));
        m_viewportBackgroundStyleCombo->addItem(trLabel("Linear gradient"), QStringLiteral("linear"));
        m_viewportBackgroundStyleCombo->addItem(trLabel("Tri-color gradient"), QStringLiteral("tricolor"));
        m_viewportBackgroundStyleCombo->addItem(trLabel("Radial gradient"), QStringLiteral("radial"));
        m_viewportBackgroundStyleCombo->addItem(trLabel("Pattern (4 corners)"), QStringLiteral("quad"));
        colorsLayout->addWidget(m_viewportBackgroundStyleCombo, colorsRow++, 1);
        m_viewportStylePreviewLabel = new QLabel(colorsTab);
        m_viewportStylePreviewLabel->setObjectName(QStringLiteral("CommandTabViewportStylePreview"));
        m_viewportStylePreviewLabel->setAlignment(Qt::AlignCenter);
        m_viewportStylePreviewLabel->setFrameShape(QFrame::NoFrame);
        m_viewportStylePreviewLabel->setMinimumHeight(58);
        colorsLayout->addWidget(m_viewportStylePreviewLabel, colorsRow++, 0, 1, 2);

        colorsLayout->addWidget(new QLabel(trLabel("Background top color"), colorsTab), colorsRow, 0);
        m_viewportBgTopButton = new QPushButton(colorsTab);
        m_viewportBgTopButton->setMinimumWidth(170);
        colorsLayout->addWidget(m_viewportBgTopButton, colorsRow++, 1);

        colorsLayout->addWidget(new QLabel(trLabel("Background middle color"), colorsTab), colorsRow, 0);
        m_viewportBgMidButton = new QPushButton(colorsTab);
        m_viewportBgMidButton->setMinimumWidth(170);
        colorsLayout->addWidget(m_viewportBgMidButton, colorsRow++, 1);

        colorsLayout->addWidget(new QLabel(trLabel("Background bottom color"), colorsTab), colorsRow, 0);
        m_viewportBgBottomButton = new QPushButton(colorsTab);
        m_viewportBgBottomButton->setMinimumWidth(170);
        colorsLayout->addWidget(m_viewportBgBottomButton, colorsRow++, 1);

        colorsLayout->addWidget(new QLabel(trLabel("Background accent color"), colorsTab), colorsRow, 0);
        m_viewportBgAccentButton = new QPushButton(colorsTab);
        m_viewportBgAccentButton->setMinimumWidth(170);
        colorsLayout->addWidget(m_viewportBgAccentButton, colorsRow++, 1);

        auto* resetViewportButton = new QPushButton(trLabel("Reset viewport to defaults"), colorsTab);
        colorsLayout->addWidget(resetViewportButton, colorsRow++, 0, 1, 2);

        // ── Grid ────────────────────────────────────────────────────────
        auto* gridSep = new QFrame(colorsTab);
        gridSep->setFrameShape(QFrame::HLine);
        gridSep->setFrameShadow(QFrame::Sunken);
        colorsLayout->addWidget(gridSep, colorsRow++, 0, 1, 2);

        m_gridColorEnabledCheck = new QCheckBox(trLabel("Override grid color"), colorsTab);
        m_gridColorEnabledCheck->setToolTip(
            trLabel("Set a custom color for the Draft grid lines.")
        );
        colorsLayout->addWidget(m_gridColorEnabledCheck, colorsRow++, 0, 1, 2);

        colorsLayout->addWidget(new QLabel(trLabel("Grid color"), colorsTab), colorsRow, 0);
        m_viewportGridColorButton = new QPushButton(colorsTab);
        m_viewportGridColorButton->setMinimumWidth(170);
        colorsLayout->addWidget(m_viewportGridColorButton, colorsRow++, 1);

        auto* resetGridButton = new QPushButton(trLabel("Reset grid to defaults"), colorsTab);
        colorsLayout->addWidget(resetGridButton, colorsRow++, 0, 1, 2);

        colorsLayout->setRowStretch(colorsRow, 1);

        // connections — viewport
        connect(m_viewportColorsEnabledCheck, &QCheckBox::toggled, this,
            [this](bool) { refreshCustomColorControls(); });
        connect(m_nativeThemeModeCombo, &QComboBox::currentIndexChanged, this,
            [this](int) { refreshCustomColorControls(); });
        connect(m_viewportBackgroundStyleCombo, &QComboBox::currentIndexChanged, this,
            [this](int) { refreshCustomColorControls(); });
        connect(m_viewportBgTopButton, &QPushButton::clicked, this, [this]() {
            const QColor initial = m_viewportBgTopColor.isValid()
                ? m_viewportBgTopColor : QColor(QStringLiteral("#192833"));
            const QColor picked = QColorDialog::getColor(initial, this,
                QCoreApplication::translate("CommandTabSettingsDialog", "Choose top background color"),
                QColorDialog::DontUseNativeDialog);
            if (!picked.isValid()) return;
            m_viewportBgTopColor = picked;
            refreshCustomColorControls();
        });
        connect(m_viewportBgMidButton, &QPushButton::clicked, this, [this]() {
            const QColor initial = m_viewportBgMidColor.isValid()
                ? m_viewportBgMidColor : QColor(QStringLiteral("#131f29"));
            const QColor picked = QColorDialog::getColor(initial, this,
                QCoreApplication::translate("CommandTabSettingsDialog", "Choose middle background color"),
                QColorDialog::DontUseNativeDialog);
            if (!picked.isValid()) return;
            m_viewportBgMidColor = picked;
            refreshCustomColorControls();
        });
        connect(m_viewportBgBottomButton, &QPushButton::clicked, this, [this]() {
            const QColor initial = m_viewportBgBottomColor.isValid()
                ? m_viewportBgBottomColor : QColor(QStringLiteral("#0a0e12"));
            const QColor picked = QColorDialog::getColor(initial, this,
                QCoreApplication::translate("CommandTabSettingsDialog", "Choose bottom background color"),
                QColorDialog::DontUseNativeDialog);
            if (!picked.isValid()) return;
            m_viewportBgBottomColor = picked;
            refreshCustomColorControls();
        });
        connect(m_viewportBgAccentButton, &QPushButton::clicked, this, [this]() {
            const QColor initial = m_viewportBgAccentColor.isValid()
                ? m_viewportBgAccentColor : QColor(QStringLiteral("#3f5a73"));
            const QColor picked = QColorDialog::getColor(initial, this,
                QCoreApplication::translate("CommandTabSettingsDialog", "Choose accent background color"),
                QColorDialog::DontUseNativeDialog);
            if (!picked.isValid()) return;
            m_viewportBgAccentColor = picked;
            refreshCustomColorControls();
        });
        connect(resetViewportButton, &QPushButton::clicked, this, [this]() {
            m_viewportBgTopColor = QColor();
            m_viewportBgMidColor = QColor();
            m_viewportBgBottomColor = QColor();
            m_viewportBgAccentColor = QColor();
            refreshCustomColorControls();
        });

        // connections — grid
        connect(m_gridColorEnabledCheck, &QCheckBox::toggled, this,
            [this](bool) { refreshCustomColorControls(); });
        connect(m_viewportGridColorButton, &QPushButton::clicked, this, [this]() {
            const QColor initial = m_viewportGridColor.isValid()
                ? m_viewportGridColor : QColor(QStringLiteral("#404040"));
            const QColor picked = QColorDialog::getColor(initial, this,
                QCoreApplication::translate("CommandTabSettingsDialog", "Choose grid color"),
                QColorDialog::DontUseNativeDialog);
            if (!picked.isValid()) return;
            m_viewportGridColor = picked;
            refreshCustomColorControls();
        });
        connect(resetGridButton, &QPushButton::clicked, this, [this]() {
            m_viewportGridColor = QColor();
            refreshCustomColorControls();
        });

        connect(
            m_customMainColorsEnabledCheck,
            &QCheckBox::toggled,
            this,
            [this](bool) {
                refreshCustomColorControls();
            }
        );
        connect(m_customMainBackgroundButton, &QPushButton::clicked, this, [this]() {
            const QColor initial = m_customMainBackgroundColor.isValid()
                ? m_customMainBackgroundColor
                : m_theme.shellBackground;
            const QColor picked = QColorDialog::getColor(
                initial,
                this,
                QCoreApplication::translate("CommandTabSettingsDialog", "Choose main background color"),
                QColorDialog::DontUseNativeDialog
            );
            if (!picked.isValid()) {
                return;
            }
            m_customMainBackgroundColor = picked;
            refreshCustomColorControls();
        });
        connect(m_customMainTextButton, &QPushButton::clicked, this, [this]() {
            const QColor initial = m_customMainTextColor.isValid()
                ? m_customMainTextColor
                : m_theme.buttonText;
            const QColor picked = QColorDialog::getColor(
                initial,
                this,
                QCoreApplication::translate("CommandTabSettingsDialog", "Choose main text color"),
                QColorDialog::DontUseNativeDialog
            );
            if (!picked.isValid()) {
                return;
            }
            m_customMainTextColor = picked;
            refreshCustomColorControls();
        });
        connect(m_customRibbonPrimaryButton, &QPushButton::clicked, this, [this]() {
            const QColor initial = m_customRibbonPrimaryColor.isValid()
                ? m_customRibbonPrimaryColor
                : m_theme.shellBackground;
            const QColor picked = QColorDialog::getColor(
                initial,
                this,
                QCoreApplication::translate("CommandTabSettingsDialog", "Choose ribbon primary color"),
                QColorDialog::DontUseNativeDialog
            );
            if (!picked.isValid()) {
                return;
            }
            m_customRibbonPrimaryColor = picked;
            refreshCustomColorControls();
        });
        connect(m_customRibbonSecondaryButton, &QPushButton::clicked, this, [this]() {
            const QColor initial = m_customRibbonSecondaryColor.isValid()
                ? m_customRibbonSecondaryColor
                : m_theme.panelCardBackground;
            const QColor picked = QColorDialog::getColor(
                initial,
                this,
                QCoreApplication::translate("CommandTabSettingsDialog", "Choose ribbon secondary color"),
                QColorDialog::DontUseNativeDialog
            );
            if (!picked.isValid()) {
                return;
            }
            m_customRibbonSecondaryColor = picked;
            refreshCustomColorControls();
        });
        connect(m_customRibbonAccentButton, &QPushButton::clicked, this, [this]() {
            const QColor initial = m_customRibbonAccentColor.isValid()
                ? m_customRibbonAccentColor
                : m_theme.tabAccent;
            const QColor picked = QColorDialog::getColor(
                initial,
                this,
                QCoreApplication::translate("CommandTabSettingsDialog", "Choose ribbon accent color"),
                QColorDialog::DontUseNativeDialog
            );
            if (!picked.isValid()) {
                return;
            }
            m_customRibbonAccentColor = picked;
            refreshCustomColorControls();
        });
        connect(resetColorsButton, &QPushButton::clicked, this, [this]() {
            m_customMainBackgroundColor = QColor();
            m_customMainTextColor = QColor();
            m_customRibbonPrimaryColor = QColor();
            m_customRibbonSecondaryColor = QColor();
            m_customRibbonAccentColor = QColor();
            refreshCustomColorControls();
        });

        m_tabs = new QTabWidget(this);
        m_tabs->setObjectName(QStringLiteral("CommandTabSettingsTabs"));
        m_tabs->setDocumentMode(true);
        auto makeScrollableTab = [this](QWidget* content) -> QScrollArea* {
            auto* scroll = new QScrollArea(this);
            scroll->setObjectName(QStringLiteral("CommandTabSettingsScroll"));
            scroll->setWidgetResizable(true);
            scroll->setFrameShape(QFrame::NoFrame);
            scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            scroll->setWidget(content);
            return scroll;
        };

        m_tabs->addTab(makeScrollableTab(generalTab), trLabel("General"));
        m_tabs->addTab(makeScrollableTab(commandtabLayoutTab), trLabel("CommandTab"));
        m_tabs->addTab(makeScrollableTab(toolsTab), trLabel("Tools"));
        m_tabs->addTab(sketcherTab, trLabel("Sketcher"));
        m_tabs->addTab(makeScrollableTab(colorsTab), trLabel("Colors"));
        m_tabs->addTab(profileTab, trLabel("Profile"));
        rootLayout->addWidget(m_tabs, 1);

        auto* footerLayout = new QHBoxLayout();
        footerLayout->setContentsMargins(0, 0, 0, 0);
        footerLayout->setSpacing(8);
        footerLayout->addStretch(1);
        auto* applyButton = new QPushButton(trLabel("Apply"), this);
        auto* saveButton = new QPushButton(trLabel("Save and Close"), this);
        auto* closeButton = new QPushButton(trLabel("Close"), this);
        footerLayout->addWidget(applyButton);
        footerLayout->addWidget(saveButton);
        footerLayout->addWidget(closeButton);
        rootLayout->addLayout(footerLayout);

        connect(applyButton, &QPushButton::clicked, this, [this]() { submit(QStringLiteral("__commandtab_preferences_apply__")); });
        connect(saveButton, &QPushButton::clicked, this, [this]() {
            submit(QStringLiteral("__commandtab_preferences_save__"));
            close();
        });
        connect(closeButton, &QPushButton::clicked, this, &QDialog::close);

        refreshCustomColorControls();
        resize(QSize(700, 540));
        setMinimumSize(QSize(620, 440));
    }

    void setTheme(const CommandTabModel::CommandTabTheme& theme)
    {
        m_theme = theme;
        const QColor textColor = theme.buttonText;
        const QColor fieldText = textColor;
        const QColor mutedText = textColor;
        const QColor selectedSurface = withAlpha(
            blendColors(theme.quickHoverBackground, theme.panelCardBackground, 0.22),
            theme.isDark ? 240 : 248
        );
        const QColor selectedText = textColor;

        auto applyTextPalette = [textColor](QPalette& palette) {
            const QPalette::ColorGroup groups[] = {
                QPalette::Active,
                QPalette::Inactive,
                QPalette::Disabled,
            };
            for (QPalette::ColorGroup group : groups) {
                palette.setColor(group, QPalette::WindowText, textColor);
                palette.setColor(group, QPalette::Text, textColor);
                palette.setColor(group, QPalette::ButtonText, textColor);
                palette.setColor(group, QPalette::ToolTipText, textColor);
                palette.setColor(group, QPalette::HighlightedText, textColor);
            }
        };

        QPalette dialogPalette = palette();
        applyTextPalette(dialogPalette);
        setPalette(dialogPalette);

        setStyleSheet(
            QStringLiteral(R"(
QDialog#CommandTabSettingsDialog {
    background: %1;
    color: %2;
}
QDialog#CommandTabSettingsDialog,
QDialog#CommandTabSettingsDialog * {
    color: %2;
}
QLabel#CommandTabSettingsHeader {
    font-size: 18px;
    font-weight: 600;
    color: %2;
}
QLabel, QCheckBox, QRadioButton {
    color: %2;
}
QCheckBox {
    spacing: 10px;
    min-height: 28px;
}
QCheckBox::indicator {
    width: 16px;
    height: 16px;
}
QTabWidget#CommandTabSettingsTabs::pane {
    border: 1px solid %3;
    border-radius: 8px;
    background: %4;
    top: -1px;
}
QTabWidget#CommandTabSettingsTabs QTabBar::tab {
    min-height: 26px;
    min-width: 84px;
    padding: 4px 10px;
    margin-right: 4px;
    border: 1px solid %3;
    border-bottom: none;
    border-top-left-radius: 7px;
    border-top-right-radius: 7px;
    background: %4;
    color: %2;
}
QTabWidget#CommandTabSettingsTabs QTabBar::tab:selected {
    background: %8;
    color: %9;
    border-color: %6;
}
QTabWidget#CommandTabSettingsTabs QTabBar::tab:hover:!selected {
    border-color: %6;
}
QComboBox, QSpinBox, QPushButton {
    min-height: 28px;
    border: 1px solid %3;
    border-radius: 6px;
    padding: 4px 8px;
    background: %4;
    color: %5;
}
QPushButton:hover, QComboBox:hover, QSpinBox:hover {
    border-color: %6;
}
QLabel:disabled, QCheckBox:disabled, QRadioButton:disabled,
QPushButton:disabled, QComboBox:disabled, QSpinBox:disabled {
    color: %7;
}
QComboBox QAbstractItemView {
    background: %4;
    color: %5;
    border: 1px solid %3;
    border-radius: 6px;
    selection-background-color: %8;
    selection-color: %9;
}
QAbstractScrollArea {
    background: %4;
    border: 1px solid %3;
    border-radius: 6px;
}
QScrollArea#CommandTabSettingsScroll {
    background: transparent;
    border: none;
}
QAbstractScrollArea > QWidget {
    background: %4;
    border-radius: 5px;
}
QSpinBox::up-button, QSpinBox::down-button {
    subcontrol-origin: border;
    width: 18px;
}
)")
                .arg(theme.shellBackground.name())
                .arg(textColor.name(QColor::HexArgb))
                .arg(theme.buttonBorder.name())
                .arg(theme.quickBackground.name())
                .arg(fieldText.name(QColor::HexArgb))
                .arg(theme.buttonActiveBorder.name())
                .arg(mutedText.name(QColor::HexArgb))
                .arg(selectedSurface.name(QColor::HexArgb))
                .arg(selectedText.name(QColor::HexArgb))
        );
        refreshCustomColorControls();
    }

    void setState(const CommandTabSettingsState& state)
    {
        m_preferNativeCommandTabCheck->setChecked(state.preferNativeCommandTab);
        m_nativeCommandTabWarmupCheck->setChecked(state.nativeCommandTabWarmup);
        m_modernCommandTabStyleCheck->setChecked(state.modernCommandTabStyleEnabled);
        m_hideMenuBarInNativeModeCheck->setChecked(state.hideMenuBarInNativeMode);
        m_ribbonAutoHideCheck->setChecked(state.ribbonAutoHide);
        m_ribbonAutoHideDelaySpinner->setValue(state.ribbonAutoHideDelayMs);
        m_ribbonHoverTabCheck->setChecked(state.ribbonHoverTab);
        m_tabClickPopupModeCheck->setChecked(state.tabClickPopupMode);
        m_compactPanelLayoutCheck->setChecked(state.compactPanelLayout);
        m_panelDropdownModeCheck->setChecked(state.panelDropdownModeEnabled);
        m_panelDropdownPrimaryRecentCheck->setChecked(state.panelDropdownPrimaryRecent);
        m_panelDropdownRecentToolCountSpin->setValue(state.panelDropdownRecentToolCount);
        m_panelDropdownPopupColumnsSpin->setValue(state.panelDropdownPopupColumns);
        m_panelDropdownPopupIconSizeSpin->setValue(state.panelDropdownPopupIconSize);
        m_headerScalePercentSpin->setValue(state.headerScalePercent);
        m_commandtabScalePercentSpin->setValue(state.commandtabScalePercent);
        m_panelDropdownPopupShowTextCheck->setChecked(state.panelDropdownPopupShowText);
        m_panelDropdownPrimaryRecentCheck->setEnabled(state.panelDropdownModeEnabled);
        m_panelDropdownRecentToolCountSpin->setEnabled(state.panelDropdownModeEnabled);
        m_panelDropdownPopupColumnsSpin->setEnabled(state.panelDropdownModeEnabled);
        m_panelDropdownPopupIconSizeSpin->setEnabled(state.panelDropdownModeEnabled);
        m_panelDropdownPopupShowTextCheck->setEnabled(state.panelDropdownModeEnabled);
        m_compactPanelSpacingSpin->setValue(state.compactPanelSpacing);
        m_compactButtonPaddingSpin->setValue(state.compactButtonPadding);
        m_showIconTextSmallCheck->setChecked(state.showIconTextSmall);
        m_showIconTextMediumCheck->setChecked(state.showIconTextMedium);
        m_showIconTextLargeCheck->setChecked(state.showIconTextLarge);
        m_iconOnlySizeSmallSpin->setValue(state.iconOnlySizeSmall);
        m_iconOnlySizeMediumSpin->setValue(state.iconOnlySizeMedium);
        m_iconOnlySizeLargeSpin->setValue(state.iconOnlySizeLarge);
        m_showSketcherGridCheck->setChecked(state.showSketcherGrid);
        m_snapSketcherGridCheck->setChecked(state.snapSketcherGrid);

        int nativeThemeIndex = m_nativeThemeModeCombo->findData(state.nativeThemeMode);
        if (nativeThemeIndex < 0) {
            nativeThemeIndex = 0;
        }
        m_nativeThemeModeCombo->setCurrentIndex(nativeThemeIndex);
        m_customMainColorsEnabledCheck->setChecked(state.customMainColorsEnabled);
        m_customMainBackgroundColor = QColor(state.customMainBackgroundColor.trimmed());
        if (!m_customMainBackgroundColor.isValid()) {
            m_customMainBackgroundColor = QColor();
        }
        m_customMainTextColor = QColor(state.customMainTextColor.trimmed());
        if (!m_customMainTextColor.isValid()) {
            m_customMainTextColor = QColor();
        }
        m_customRibbonPrimaryColor = QColor(state.customRibbonPrimaryColor.trimmed());
        if (!m_customRibbonPrimaryColor.isValid()) {
            m_customRibbonPrimaryColor = QColor();
        }
        m_customRibbonSecondaryColor = QColor(state.customRibbonSecondaryColor.trimmed());
        if (!m_customRibbonSecondaryColor.isValid()) {
            m_customRibbonSecondaryColor = QColor();
        }
        m_customRibbonAccentColor = QColor(state.customRibbonAccentColor.trimmed());
        if (!m_customRibbonAccentColor.isValid()) {
            m_customRibbonAccentColor = QColor();
        }
        m_viewportColorsEnabledCheck->setChecked(state.viewportColorsEnabled);
        int viewportBackgroundStyleIndex = m_viewportBackgroundStyleCombo->findData(
            state.viewportBackgroundStyle
        );
        if (viewportBackgroundStyleIndex < 0) {
            viewportBackgroundStyleIndex = m_viewportBackgroundStyleCombo->findData(
                QStringLiteral("linear")
            );
        }
        if (viewportBackgroundStyleIndex < 0) {
            viewportBackgroundStyleIndex = 0;
        }
        m_viewportBackgroundStyleCombo->setCurrentIndex(viewportBackgroundStyleIndex);
        m_viewportBgTopColor = QColor(state.viewportBgTopColor.trimmed());
        if (!m_viewportBgTopColor.isValid()) m_viewportBgTopColor = QColor();
        m_viewportBgMidColor = QColor(state.viewportBgMidColor.trimmed());
        if (!m_viewportBgMidColor.isValid()) m_viewportBgMidColor = QColor();
        m_viewportBgBottomColor = QColor(state.viewportBgBottomColor.trimmed());
        if (!m_viewportBgBottomColor.isValid()) m_viewportBgBottomColor = QColor();
        m_viewportBgAccentColor = QColor(state.viewportBgAccentColor.trimmed());
        if (!m_viewportBgAccentColor.isValid()) m_viewportBgAccentColor = QColor();
        m_gridColorEnabledCheck->setChecked(state.gridColorEnabled);
        m_viewportGridColor = QColor(state.viewportGridColor.trimmed());
        if (!m_viewportGridColor.isValid()) m_viewportGridColor = QColor();
        refreshCustomColorControls();
    }

    void setCommandHandler(std::function<void(const QString&)> handler)
    {
        m_commandHandler = std::move(handler);
    }

private:
    QString colorHexOrEmpty(const QColor& color) const
    {
        return color.isValid() ? color.name(QColor::HexRgb) : QString();
    }

    void refreshColorButton(
        QPushButton* button,
        const QColor& selectedColor,
        const QColor& fallbackColor,
        const QString& defaultLabel
    )
    {
        if (button == nullptr) {
            return;
        }

        const QColor swatchColor = selectedColor.isValid() ? selectedColor : fallbackColor;
        const QColor safeSwatchColor = swatchColor.isValid()
            ? swatchColor
            : QColor(QStringLiteral("#808080"));
        const QColor textColor = ensureReadableTextColor(
            safeSwatchColor,
            QColor(QStringLiteral("#111111")),
            4.6,
            QColor(QStringLiteral("#0f1621")),
            QColor(QStringLiteral("#f6f8fd"))
        );
        const QColor borderColor = withAlpha(textColor, 96);
        const QColor hoverBorderColor = withAlpha(textColor, 156);
        const QColor disabledBackground = withAlpha(safeSwatchColor, 144);
        const QColor disabledText = withAlpha(textColor, 156);
        const QColor disabledBorder = withAlpha(textColor, 60);

        button->setText(selectedColor.isValid() ? selectedColor.name(QColor::HexRgb) : defaultLabel);
        button->setStyleSheet(
            QStringLiteral(R"(
QPushButton {
    min-height: 30px;
    font-weight: 600;
    border-radius: 6px;
    border: 1px solid %3;
    background: %1;
    color: %2;
}
QPushButton:hover {
    border-color: %4;
}
QPushButton:disabled {
    background: %5;
    color: %6;
    border-color: %7;
}
)")
                .arg(safeSwatchColor.name(QColor::HexArgb))
                .arg(textColor.name(QColor::HexArgb))
                .arg(borderColor.name(QColor::HexArgb))
                .arg(hoverBorderColor.name(QColor::HexArgb))
                .arg(disabledBackground.name(QColor::HexArgb))
                .arg(disabledText.name(QColor::HexArgb))
                .arg(disabledBorder.name(QColor::HexArgb))
        );
    }

    CommandTabModel::CommandTabTheme previewThemeFromSelection() const
    {
        CommandTabModel::CommandTabTheme previewTheme = m_theme;
        if (m_customMainColorsEnabledCheck != nullptr && m_customMainColorsEnabledCheck->isChecked()) {
            CommandTabSettingsState overrideState;
            overrideState.customMainColorsEnabled = true;
            overrideState.customMainBackgroundColor = colorHexOrEmpty(m_customMainBackgroundColor);
            overrideState.customMainTextColor = colorHexOrEmpty(m_customMainTextColor);
            overrideState.customRibbonPrimaryColor = colorHexOrEmpty(m_customRibbonPrimaryColor);
            overrideState.customRibbonSecondaryColor = colorHexOrEmpty(m_customRibbonSecondaryColor);
            overrideState.customRibbonAccentColor = colorHexOrEmpty(m_customRibbonAccentColor);
            previewTheme = applySettingsThemeOverrides(previewTheme, overrideState);
        }
        return normalizeShellTheme(previewTheme);
    }

    QColor previewReadableTextColor(
        const CommandTabModel::CommandTabTheme& theme,
        const QColor& background,
        qreal contrast = 4.6
    ) const
    {
        if (theme.forceTextColor && theme.buttonText.isValid()) {
            return theme.buttonText;
        }
        return ensureReadableTextColor(background, theme.buttonText, contrast);
    }

    QPixmap previewLogoPixmap(
        int edge,
        const QColor& surface,
        const QColor& border,
        const QColor& accent,
        const QColor& textColor,
        bool rightVariant
    ) const
    {
        const int safeEdge = std::max(16, edge);
        QPixmap pixmap(safeEdge, safeEdge);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF rect(0.5, 0.5, safeEdge - 1.0, safeEdge - 1.0);

        QLinearGradient surfaceGradient(0.0, rect.top(), 0.0, rect.bottom());
        surfaceGradient.setColorAt(0.0, withAlpha(blendColors(surface, QColor(QStringLiteral("#ffffff")), 0.22), 245));
        surfaceGradient.setColorAt(1.0, withAlpha(blendColors(surface, QColor(QStringLiteral("#0f1724")), 0.12), 228));
        painter.setBrush(surfaceGradient);
        painter.setPen(QPen(withAlpha(border, 208), 1.0));
        painter.drawRoundedRect(rect, 6.0, 6.0);

        QLinearGradient shineGradient(0.0, rect.top(), rect.width(), rect.bottom());
        shineGradient.setColorAt(0.0, withAlpha(QColor(QStringLiteral("#ffffff")), rightVariant ? 118 : 142));
        shineGradient.setColorAt(0.7, withAlpha(accent, rightVariant ? 76 : 92));
        shineGradient.setColorAt(1.0, withAlpha(accent, 28));
        painter.setPen(Qt::NoPen);
        painter.setBrush(shineGradient);
        painter.drawRoundedRect(rect.adjusted(1.5, 1.5, -1.5, -(safeEdge * 0.36)), 5.0, 5.0);

        if (rightVariant) {
            painter.setPen(QPen(withAlpha(accent, 212), 1.4));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(
                QPointF(rect.center().x(), rect.center().y()),
                safeEdge * 0.22,
                safeEdge * 0.22
            );
            painter.setPen(QPen(withAlpha(accent, 156), 1.1));
            painter.drawLine(
                QPointF(rect.center().x() - safeEdge * 0.25, rect.center().y()),
                QPointF(rect.center().x() + safeEdge * 0.25, rect.center().y())
            );
        } else {
            painter.setPen(QPen(withAlpha(accent, 212), 1.5));
            painter.drawLine(
                QPointF(rect.left() + safeEdge * 0.30, rect.bottom() - safeEdge * 0.28),
                QPointF(rect.right() - safeEdge * 0.24, rect.top() + safeEdge * 0.24)
            );
            painter.drawLine(
                QPointF(rect.left() + safeEdge * 0.27, rect.center().y()),
                QPointF(rect.right() - safeEdge * 0.28, rect.center().y())
            );
        }

        painter.setPen(withAlpha(textColor, 230));
        QFont logoFont = painter.font();
        logoFont.setPointSizeF(std::max(7.0, safeEdge * 0.34));
        logoFont.setBold(true);
        painter.setFont(logoFont);
        painter.drawText(rect.toRect(), Qt::AlignCenter, rightVariant ? QStringLiteral("C") : QStringLiteral("F"));
        return pixmap;
    }

    void setComboItemIconByData(QComboBox* combo, const QString& dataKey, const QIcon& icon) const
    {
        if (combo == nullptr) {
            return;
        }
        const int index = combo->findData(dataKey);
        if (index >= 0) {
            combo->setItemIcon(index, icon);
        }
    }

    QIcon makeThemeModePreviewIcon(const QString& modeKey) const
    {
        const QSize iconSize(44, 22);
        QPixmap pixmap(iconSize);
        pixmap.fill(Qt::transparent);

        const bool dark = modeKey == QStringLiteral("dark");
        const bool light = modeKey == QStringLiteral("light");
        const bool autoMode = modeKey == QStringLiteral("auto");
        const QColor border = withAlpha(m_theme.buttonBorder.isValid() ? m_theme.buttonBorder : QColor(QStringLiteral("#6b7482")), 210);
        const QColor accent = m_theme.tabAccent.isValid() ? m_theme.tabAccent : QColor(QStringLiteral("#3b82f6"));

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF outer(0.5, 0.5, iconSize.width() - 1.0, iconSize.height() - 1.0);
        painter.setPen(QPen(border, 1.0));

        if (autoMode) {
            QLinearGradient split(outer.left(), outer.top(), outer.right(), outer.top());
            split.setColorAt(0.0, QColor(QStringLiteral("#1a1f27")));
            split.setColorAt(0.48, QColor(QStringLiteral("#2a3340")));
            split.setColorAt(0.52, QColor(QStringLiteral("#eaf1fb")));
            split.setColorAt(1.0, QColor(QStringLiteral("#ffffff")));
            painter.setBrush(split);
            painter.drawRoundedRect(outer, 5.0, 5.0);
            painter.setPen(QPen(withAlpha(accent, 210), 1.1));
            painter.drawLine(
                QPointF(outer.center().x(), outer.top() + 2.0),
                QPointF(outer.center().x(), outer.bottom() - 2.0)
            );
            painter.setPen(withAlpha(QColor(QStringLiteral("#ffffff")), 236));
            QFont f = painter.font();
            f.setBold(true);
            f.setPointSize(8);
            painter.setFont(f);
            painter.drawText(outer.adjusted(0, 0, 0, 0).toRect(), Qt::AlignCenter, QStringLiteral("A"));
            return QIcon(pixmap);
        }

        QLinearGradient surface(outer.left(), outer.top(), outer.left(), outer.bottom());
        if (dark) {
            surface.setColorAt(0.0, QColor(QStringLiteral("#313b47")));
            surface.setColorAt(1.0, QColor(QStringLiteral("#171c23")));
        } else if (light) {
            surface.setColorAt(0.0, QColor(QStringLiteral("#ffffff")));
            surface.setColorAt(1.0, QColor(QStringLiteral("#e8eef6")));
        } else {
            surface.setColorAt(0.0, QColor(QStringLiteral("#596170")));
            surface.setColorAt(1.0, QColor(QStringLiteral("#2f3642")));
        }
        painter.setBrush(surface);
        painter.drawRoundedRect(outer, 5.0, 5.0);

        QLinearGradient sheen(outer.left(), outer.top(), outer.left(), outer.top() + outer.height() * 0.58);
        sheen.setColorAt(0.0, withAlpha(QColor(QStringLiteral("#ffffff")), dark ? 90 : 128));
        sheen.setColorAt(1.0, withAlpha(QColor(QStringLiteral("#ffffff")), 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(sheen);
        painter.drawRoundedRect(outer.adjusted(1.5, 1.5, -1.5, -outer.height() * 0.40), 4.0, 4.0);

        painter.setPen(withAlpha(accent, 214));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(outer.adjusted(1.0, 1.0, -1.0, -1.0), 4.0, 4.0);
        return QIcon(pixmap);
    }

    QIcon makeViewportStylePreviewIcon(const QString& styleKey) const
    {
        const QSize iconSize(44, 22);
        QPixmap pixmap(iconSize);
        pixmap.fill(Qt::transparent);

        const QColor top = m_viewportBgTopColor.isValid() ? m_viewportBgTopColor : QColor(QStringLiteral("#192833"));
        const QColor mid = m_viewportBgMidColor.isValid() ? m_viewportBgMidColor : QColor(QStringLiteral("#131f29"));
        const QColor bottom = m_viewportBgBottomColor.isValid() ? m_viewportBgBottomColor : QColor(QStringLiteral("#0a0e12"));
        const QColor accent = m_viewportBgAccentColor.isValid() ? m_viewportBgAccentColor : QColor(QStringLiteral("#3f5a73"));
        const QColor border = withAlpha(m_theme.buttonBorder.isValid() ? m_theme.buttonBorder : QColor(QStringLiteral("#6b7482")), 210);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF outer(0.5, 0.5, iconSize.width() - 1.0, iconSize.height() - 1.0);
        QPainterPath clip;
        clip.addRoundedRect(outer, 5.0, 5.0);
        painter.setClipPath(clip);

        if (styleKey == QStringLiteral("solid")) {
            painter.fillRect(outer, top);
        } else if (styleKey == QStringLiteral("linear")) {
            QLinearGradient gradient(outer.left(), outer.top(), outer.left(), outer.bottom());
            gradient.setColorAt(0.0, top);
            gradient.setColorAt(1.0, bottom);
            painter.fillRect(outer, gradient);
        } else if (styleKey == QStringLiteral("tricolor")) {
            QLinearGradient gradient(outer.left(), outer.top(), outer.left(), outer.bottom());
            gradient.setColorAt(0.0, top);
            gradient.setColorAt(0.5, mid);
            gradient.setColorAt(1.0, bottom);
            painter.fillRect(outer, gradient);
        } else if (styleKey == QStringLiteral("radial")) {
            painter.fillRect(outer, bottom);
            QRadialGradient radial(outer.center(), std::max(outer.width(), outer.height()) * 0.70);
            radial.setColorAt(0.0, mid);
            radial.setColorAt(0.68, top);
            radial.setColorAt(1.0, bottom);
            painter.fillRect(outer, radial);
        } else if (styleKey == QStringLiteral("quad")) {
            painter.fillRect(outer, bottom);
            const qreal radius = std::max(outer.width(), outer.height()) * 0.75;
            QRadialGradient tl(QPointF(outer.left(), outer.top()), radius);
            tl.setColorAt(0.0, top);
            tl.setColorAt(1.0, withAlpha(top, 0));
            painter.fillRect(outer, tl);
            QRadialGradient tr(QPointF(outer.right(), outer.top()), radius);
            tr.setColorAt(0.0, mid);
            tr.setColorAt(1.0, withAlpha(mid, 0));
            painter.fillRect(outer, tr);
            QRadialGradient bl(QPointF(outer.left(), outer.bottom()), radius);
            bl.setColorAt(0.0, accent);
            bl.setColorAt(1.0, withAlpha(accent, 0));
            painter.fillRect(outer, bl);
            QRadialGradient br(QPointF(outer.right(), outer.bottom()), radius);
            br.setColorAt(0.0, bottom);
            br.setColorAt(1.0, withAlpha(bottom, 0));
            painter.fillRect(outer, br);
        } else {
            painter.fillRect(outer, top);
        }

        painter.setClipping(false);
        painter.setPen(QPen(border, 1.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(outer, 5.0, 5.0);
        return QIcon(pixmap);
    }

    QPixmap makeThemeModeShowcasePixmap(const QString& modeKey) const
    {
        const QSize size(244, 56);
        QPixmap pixmap(size);
        pixmap.fill(Qt::transparent);

        const bool dark = modeKey == QStringLiteral("dark");
        const bool light = modeKey == QStringLiteral("light");
        const bool autoMode = modeKey == QStringLiteral("auto");
        const QColor accent = m_theme.tabAccent.isValid() ? m_theme.tabAccent : QColor(QStringLiteral("#3b82f6"));

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF outer(0.5, 0.5, size.width() - 1.0, size.height() - 1.0);
        painter.setPen(QPen(withAlpha(m_theme.buttonBorder.isValid() ? m_theme.buttonBorder : QColor(QStringLiteral("#6b7482")), 210), 1.0));

        if (autoMode) {
            QLinearGradient split(outer.left(), outer.top(), outer.right(), outer.top());
            split.setColorAt(0.0, QColor(QStringLiteral("#171c23")));
            split.setColorAt(0.46, QColor(QStringLiteral("#2c3644")));
            split.setColorAt(0.54, QColor(QStringLiteral("#edf3fb")));
            split.setColorAt(1.0, QColor(QStringLiteral("#ffffff")));
            painter.setBrush(split);
            painter.drawRoundedRect(outer, 8.0, 8.0);
            painter.setPen(QPen(withAlpha(accent, 226), 1.4));
            painter.drawLine(
                QPointF(outer.center().x(), outer.top() + 4.0),
                QPointF(outer.center().x(), outer.bottom() - 4.0)
            );
            painter.setPen(withAlpha(QColor(QStringLiteral("#ffffff")), 238));
            QFont f = painter.font();
            f.setBold(true);
            f.setPointSize(10);
            painter.setFont(f);
            painter.drawText(outer.toRect(), Qt::AlignCenter, QStringLiteral("Auto"));
            return pixmap;
        }

        QLinearGradient surface(outer.left(), outer.top(), outer.left(), outer.bottom());
        if (dark) {
            surface.setColorAt(0.0, QColor(QStringLiteral("#2c3541")));
            surface.setColorAt(1.0, QColor(QStringLiteral("#151a21")));
        } else if (light) {
            surface.setColorAt(0.0, QColor(QStringLiteral("#ffffff")));
            surface.setColorAt(1.0, QColor(QStringLiteral("#e6edf7")));
        } else {
            surface.setColorAt(0.0, QColor(QStringLiteral("#4e5969")));
            surface.setColorAt(1.0, QColor(QStringLiteral("#2f3642")));
        }
        painter.setBrush(surface);
        painter.drawRoundedRect(outer, 8.0, 8.0);

        QLinearGradient shine(outer.left(), outer.top(), outer.left(), outer.top() + outer.height() * 0.60);
        shine.setColorAt(0.0, withAlpha(QColor(QStringLiteral("#ffffff")), dark ? 86 : 132));
        shine.setColorAt(1.0, withAlpha(QColor(QStringLiteral("#ffffff")), 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(shine);
        painter.drawRoundedRect(outer.adjusted(2.0, 2.0, -2.0, -outer.height() * 0.42), 7.0, 7.0);

        painter.setPen(QPen(withAlpha(accent, 214), 1.2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(outer.adjusted(1.5, 1.5, -1.5, -1.5), 7.0, 7.0);
        painter.setPen(withAlpha(dark ? QColor(QStringLiteral("#e7edf7")) : QColor(QStringLiteral("#101722")), 235));
        QFont f = painter.font();
        f.setBold(true);
        f.setPointSize(10);
        painter.setFont(f);
        painter.drawText(outer.toRect(), Qt::AlignCenter, dark ? QStringLiteral("Dark") : QStringLiteral("Light"));
        return pixmap;
    }

    QPixmap makeViewportStyleShowcasePixmap(const QString& styleKey) const
    {
        const QSize size(244, 56);
        QPixmap pixmap(size);
        pixmap.fill(Qt::transparent);

        const QColor top = m_viewportBgTopColor.isValid() ? m_viewportBgTopColor : QColor(QStringLiteral("#192833"));
        const QColor mid = m_viewportBgMidColor.isValid() ? m_viewportBgMidColor : QColor(QStringLiteral("#131f29"));
        const QColor bottom = m_viewportBgBottomColor.isValid() ? m_viewportBgBottomColor : QColor(QStringLiteral("#0a0e12"));
        const QColor accent = m_viewportBgAccentColor.isValid() ? m_viewportBgAccentColor : QColor(QStringLiteral("#3f5a73"));

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF outer(0.5, 0.5, size.width() - 1.0, size.height() - 1.0);
        QPainterPath clip;
        clip.addRoundedRect(outer, 8.0, 8.0);
        painter.setClipPath(clip);

        if (styleKey == QStringLiteral("solid")) {
            painter.fillRect(outer, top);
        } else if (styleKey == QStringLiteral("linear")) {
            QLinearGradient gradient(outer.left(), outer.top(), outer.left(), outer.bottom());
            gradient.setColorAt(0.0, top);
            gradient.setColorAt(1.0, bottom);
            painter.fillRect(outer, gradient);
        } else if (styleKey == QStringLiteral("tricolor")) {
            QLinearGradient gradient(outer.left(), outer.top(), outer.left(), outer.bottom());
            gradient.setColorAt(0.0, top);
            gradient.setColorAt(0.5, mid);
            gradient.setColorAt(1.0, bottom);
            painter.fillRect(outer, gradient);
        } else if (styleKey == QStringLiteral("radial")) {
            painter.fillRect(outer, bottom);
            QRadialGradient radial(outer.center(), std::max(outer.width(), outer.height()) * 0.70);
            radial.setColorAt(0.0, mid);
            radial.setColorAt(0.68, top);
            radial.setColorAt(1.0, bottom);
            painter.fillRect(outer, radial);
        } else if (styleKey == QStringLiteral("quad")) {
            painter.fillRect(outer, bottom);
            const qreal radius = std::max(outer.width(), outer.height()) * 0.72;
            QRadialGradient tl(QPointF(outer.left(), outer.top()), radius);
            tl.setColorAt(0.0, top);
            tl.setColorAt(1.0, withAlpha(top, 0));
            painter.fillRect(outer, tl);
            QRadialGradient tr(QPointF(outer.right(), outer.top()), radius);
            tr.setColorAt(0.0, mid);
            tr.setColorAt(1.0, withAlpha(mid, 0));
            painter.fillRect(outer, tr);
            QRadialGradient bl(QPointF(outer.left(), outer.bottom()), radius);
            bl.setColorAt(0.0, accent);
            bl.setColorAt(1.0, withAlpha(accent, 0));
            painter.fillRect(outer, bl);
            QRadialGradient br(QPointF(outer.right(), outer.bottom()), radius);
            br.setColorAt(0.0, bottom);
            br.setColorAt(1.0, withAlpha(bottom, 0));
            painter.fillRect(outer, br);
        } else {
            painter.fillRect(outer, top);
        }

        painter.setClipping(false);
        painter.setPen(QPen(withAlpha(m_theme.buttonBorder.isValid() ? m_theme.buttonBorder : QColor(QStringLiteral("#6b7482")), 210), 1.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(outer, 8.0, 8.0);
        return pixmap;
    }

    void refreshVisualChoiceIcons()
    {
        setComboItemIconByData(
            m_nativeThemeModeCombo,
            QStringLiteral("auto"),
            makeThemeModePreviewIcon(QStringLiteral("auto"))
        );
        setComboItemIconByData(
            m_nativeThemeModeCombo,
            QStringLiteral("dark"),
            makeThemeModePreviewIcon(QStringLiteral("dark"))
        );
        setComboItemIconByData(
            m_nativeThemeModeCombo,
            QStringLiteral("light"),
            makeThemeModePreviewIcon(QStringLiteral("light"))
        );

        setComboItemIconByData(
            m_viewportBackgroundStyleCombo,
            QStringLiteral("solid"),
            makeViewportStylePreviewIcon(QStringLiteral("solid"))
        );
        setComboItemIconByData(
            m_viewportBackgroundStyleCombo,
            QStringLiteral("linear"),
            makeViewportStylePreviewIcon(QStringLiteral("linear"))
        );
        setComboItemIconByData(
            m_viewportBackgroundStyleCombo,
            QStringLiteral("tricolor"),
            makeViewportStylePreviewIcon(QStringLiteral("tricolor"))
        );
        setComboItemIconByData(
            m_viewportBackgroundStyleCombo,
            QStringLiteral("radial"),
            makeViewportStylePreviewIcon(QStringLiteral("radial"))
        );
        setComboItemIconByData(
            m_viewportBackgroundStyleCombo,
            QStringLiteral("quad"),
            makeViewportStylePreviewIcon(QStringLiteral("quad"))
        );
    }

    void refreshVisualChoicePreviews()
    {
        if (m_nativeThemePreviewLabel != nullptr && m_nativeThemeModeCombo != nullptr) {
            const QString modeKey = m_nativeThemeModeCombo->currentData().toString().trimmed().toLower();
            m_nativeThemePreviewLabel->setPixmap(makeThemeModeShowcasePixmap(modeKey));
        }
        if (m_viewportStylePreviewLabel != nullptr && m_viewportBackgroundStyleCombo != nullptr) {
            const QString styleKey = m_viewportBackgroundStyleCombo->currentData().toString().trimmed().toLower();
            m_viewportStylePreviewLabel->setPixmap(makeViewportStyleShowcasePixmap(styleKey));
        }
    }

    QIcon makePreferenceOptionIcon(const QString& key) const
    {
        const int edge = 24;
        QPixmap pixmap(edge, edge);
        pixmap.fill(Qt::transparent);

        const QColor accent = m_theme.tabAccent.isValid() ? m_theme.tabAccent : QColor(QStringLiteral("#3b82f6"));
        const QColor surface = m_theme.isDark
            ? QColor(QStringLiteral("#1f2834"))
            : QColor(QStringLiteral("#f1f6ff"));
        const QColor border = withAlpha(
            m_theme.buttonBorder.isValid() ? m_theme.buttonBorder : QColor(QStringLiteral("#6b7482")),
            234
        );
        const QColor glyph = ensureReadableTextColor(surface, accent, 4.0);
        const QColor glyphSoft = withAlpha(glyph, 214);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF outer(0.5, 0.5, edge - 1.0, edge - 1.0);
        painter.setPen(QPen(border, 1.1));
        painter.setBrush(surface);
        painter.drawRoundedRect(outer, 5.2, 5.2);

        auto drawDots = [&](const QVector<QPointF>& dots, qreal radius) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(glyph);
            for (const QPointF& p : dots) {
                painter.drawEllipse(p, radius, radius);
            }
        };

        painter.setPen(QPen(glyph, 1.9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        if (key == QStringLiteral("native")) {
            painter.drawLine(QPointF(4.5, 12.5), QPointF(9.0, 5.0));
            painter.drawLine(QPointF(9.0, 5.0), QPointF(13.5, 12.5));
            painter.drawLine(QPointF(6.3, 9.0), QPointF(11.7, 9.0));
        } else if (key == QStringLiteral("warmup")) {
            painter.drawEllipse(QPointF(9.0, 9.0), 3.6, 3.6);
            painter.drawLine(QPointF(9.0, 9.0), QPointF(9.0, 6.9));
            painter.drawLine(QPointF(9.0, 9.0), QPointF(10.9, 10.2));
        } else if (key == QStringLiteral("style")) {
            drawDots({QPointF(7.1, 7.2), QPointF(15.0, 8.2), QPointF(11.2, 15.2)}, 1.35);
        } else if (key == QStringLiteral("menu")) {
            painter.drawLine(QPointF(5.0, 6.0), QPointF(13.0, 6.0));
            painter.drawLine(QPointF(5.0, 9.0), QPointF(13.0, 9.0));
            painter.drawLine(QPointF(5.0, 12.0), QPointF(13.0, 12.0));
        } else if (key == QStringLiteral("autohide")) {
            painter.drawLine(QPointF(5.0, 6.2), QPointF(13.0, 6.2));
            painter.drawLine(QPointF(5.0, 9.2), QPointF(11.0, 9.2));
            painter.drawLine(QPointF(5.0, 12.2), QPointF(9.0, 12.2));
            painter.drawLine(QPointF(11.6, 10.8), QPointF(13.4, 12.6));
            painter.drawLine(QPointF(11.6, 12.6), QPointF(13.4, 10.8));
        } else if (key == QStringLiteral("hover")) {
            painter.drawLine(QPointF(5.0, 6.0), QPointF(12.6, 9.0));
            painter.drawLine(QPointF(12.6, 9.0), QPointF(8.6, 12.8));
        } else if (key == QStringLiteral("popup")) {
            painter.drawRoundedRect(QRectF(4.5, 5.0, 8.0, 6.6), 2.0, 2.0);
            painter.drawLine(QPointF(7.2, 11.6), QPointF(5.8, 13.3));
        } else if (key == QStringLiteral("layout")) {
            painter.drawRect(QRectF(4.7, 5.0, 3.5, 3.5));
            painter.drawRect(QRectF(9.8, 5.0, 3.5, 3.5));
            painter.drawRect(QRectF(4.7, 10.1, 3.5, 3.5));
            painter.drawRect(QRectF(9.8, 10.1, 3.5, 3.5));
        } else if (key == QStringLiteral("dropdown")) {
            painter.drawLine(QPointF(5.5, 6.0), QPointF(12.5, 6.0));
            painter.drawLine(QPointF(8.2, 9.0), QPointF(10.0, 11.0));
            painter.drawLine(QPointF(10.0, 11.0), QPointF(11.8, 9.0));
        } else if (key == QStringLiteral("recent")) {
            painter.drawEllipse(QPointF(7.2, 9.0), 2.0, 2.0);
            painter.drawLine(QPointF(10.0, 6.2), QPointF(13.3, 6.2));
            painter.drawLine(QPointF(10.0, 9.0), QPointF(13.3, 9.0));
            painter.drawLine(QPointF(10.0, 11.8), QPointF(13.3, 11.8));
        } else if (key == QStringLiteral("text")) {
            painter.drawLine(QPointF(5.2, 6.0), QPointF(12.8, 6.0));
            painter.drawLine(QPointF(9.0, 6.0), QPointF(9.0, 12.8));
        } else if (key == QStringLiteral("size")) {
            painter.drawLine(QPointF(5.2, 12.4), QPointF(12.8, 12.4));
            painter.drawLine(QPointF(5.2, 12.4), QPointF(6.6, 11.0));
            painter.drawLine(QPointF(5.2, 12.4), QPointF(6.6, 13.8));
            painter.drawLine(QPointF(12.8, 12.4), QPointF(11.4, 11.0));
            painter.drawLine(QPointF(12.8, 12.4), QPointF(11.4, 13.8));
        } else if (key == QStringLiteral("grid")) {
            painter.setPen(QPen(glyphSoft, 1.5));
            for (int i = 0; i < 3; ++i) {
                const qreal x = 5.0 + i * 3.2;
                painter.drawLine(QPointF(x, 5.0), QPointF(x, 13.0));
                const qreal y = 5.0 + i * 3.2;
                painter.drawLine(QPointF(5.0, y), QPointF(13.0, y));
            }
        } else if (key == QStringLiteral("palette")) {
            painter.drawEllipse(QPointF(8.2, 8.7), 3.5, 3.5);
            drawDots({QPointF(6.7, 7.6), QPointF(8.4, 6.6), QPointF(9.8, 7.9)}, 0.7);
        } else if (key == QStringLiteral("viewport")) {
            painter.drawRect(QRectF(4.8, 5.4, 8.4, 6.2));
            painter.drawLine(QPointF(5.8, 6.5), QPointF(12.2, 10.5));
        } else if (key == QStringLiteral("profile")) {
            painter.drawLine(QPointF(5.2, 6.3), QPointF(12.8, 6.3));
            painter.drawLine(QPointF(5.2, 9.0), QPointF(12.8, 9.0));
            painter.drawLine(QPointF(5.2, 11.7), QPointF(10.6, 11.7));
        } else {
            painter.drawLine(QPointF(5.2, 9.0), QPointF(12.8, 9.0));
        }

        return QIcon(pixmap);
    }

    void applyIconToButton(QAbstractButton* button, const QString& iconKey)
    {
        if (button == nullptr) {
            return;
        }
        button->setIcon(makePreferenceOptionIcon(iconKey));
        if (qobject_cast<QCheckBox*>(button) != nullptr) {
            button->setIconSize(QSize(24, 24));
            return;
        }
        button->setIconSize(QSize(18, 18));
    }

    void refreshOptionRowIcons()
    {
        applyIconToButton(m_preferNativeCommandTabCheck, QStringLiteral("native"));
        applyIconToButton(m_nativeCommandTabWarmupCheck, QStringLiteral("warmup"));
        applyIconToButton(m_modernCommandTabStyleCheck, QStringLiteral("style"));
        applyIconToButton(m_hideMenuBarInNativeModeCheck, QStringLiteral("menu"));
        applyIconToButton(m_ribbonAutoHideCheck, QStringLiteral("autohide"));
        applyIconToButton(m_ribbonHoverTabCheck, QStringLiteral("hover"));
        applyIconToButton(m_tabClickPopupModeCheck, QStringLiteral("popup"));
        applyIconToButton(m_compactPanelLayoutCheck, QStringLiteral("layout"));
        applyIconToButton(m_panelDropdownModeCheck, QStringLiteral("dropdown"));
        applyIconToButton(m_panelDropdownPrimaryRecentCheck, QStringLiteral("recent"));
        applyIconToButton(m_panelDropdownPopupShowTextCheck, QStringLiteral("text"));
        applyIconToButton(m_showIconTextSmallCheck, QStringLiteral("text"));
        applyIconToButton(m_showIconTextMediumCheck, QStringLiteral("text"));
        applyIconToButton(m_showIconTextLargeCheck, QStringLiteral("text"));
        applyIconToButton(m_showSketcherGridCheck, QStringLiteral("grid"));
        applyIconToButton(m_snapSketcherGridCheck, QStringLiteral("grid"));
        applyIconToButton(m_customMainColorsEnabledCheck, QStringLiteral("palette"));
        applyIconToButton(m_viewportColorsEnabledCheck, QStringLiteral("viewport"));
        applyIconToButton(m_gridColorEnabledCheck, QStringLiteral("grid"));

        applyIconToButton(m_customMainBackgroundButton, QStringLiteral("palette"));
        applyIconToButton(m_customMainTextButton, QStringLiteral("text"));
        applyIconToButton(m_customRibbonPrimaryButton, QStringLiteral("palette"));
        applyIconToButton(m_customRibbonSecondaryButton, QStringLiteral("palette"));
        applyIconToButton(m_customRibbonAccentButton, QStringLiteral("palette"));
        applyIconToButton(m_viewportBgTopButton, QStringLiteral("viewport"));
        applyIconToButton(m_viewportBgMidButton, QStringLiteral("viewport"));
        applyIconToButton(m_viewportBgBottomButton, QStringLiteral("viewport"));
        applyIconToButton(m_viewportBgAccentButton, QStringLiteral("viewport"));
        applyIconToButton(m_viewportGridColorButton, QStringLiteral("grid"));

        if (m_tabs != nullptr) {
            m_tabs->setTabIcon(0, makePreferenceOptionIcon(QStringLiteral("native")));
            m_tabs->setTabIcon(1, makePreferenceOptionIcon(QStringLiteral("layout")));
            m_tabs->setTabIcon(2, makePreferenceOptionIcon(QStringLiteral("dropdown")));
            m_tabs->setTabIcon(3, makePreferenceOptionIcon(QStringLiteral("grid")));
            m_tabs->setTabIcon(4, makePreferenceOptionIcon(QStringLiteral("palette")));
            m_tabs->setTabIcon(5, makePreferenceOptionIcon(QStringLiteral("profile")));
        }
    }

    void refreshColorPreview()
    {
        if (
            m_colorPreviewFrame == nullptr
            || m_colorPreviewRibbonRow == nullptr
            || m_colorPreviewPanelCard == nullptr
            || m_colorPreviewTabActive == nullptr
            || m_colorPreviewTabInactive == nullptr
            || m_colorPreviewAccentBadge == nullptr
            || m_colorPreviewPanelTitle == nullptr
            || m_colorPreviewPanelBody == nullptr
            || m_colorPreviewLogoLeft == nullptr
            || m_colorPreviewLogoRight == nullptr
        ) {
            return;
        }

        const CommandTabModel::CommandTabTheme previewTheme = previewThemeFromSelection();
        const QColor shellSurface = previewTheme.shellBackground;
        const QColor panelSurface = previewTheme.panelCardBackground;
        const QColor accent = previewTheme.tabAccent;
        const QColor border = withAlpha(
            blendColors(previewTheme.shellBorder, previewTheme.panelCardBorder, 0.36),
            224
        );
        const QColor headerSurface = withAlpha(
            blendColors(shellSurface, panelSurface, 0.26),
            previewTheme.isDark ? 224 : 242
        );
        const QColor previewSurface = withAlpha(
            blendColors(shellSurface, panelSurface, 0.18),
            previewTheme.isDark ? 176 : 242
        );
        const QColor cardSurface = withAlpha(
            blendColors(panelSurface, previewTheme.panelFooterBackground, 0.24),
            previewTheme.isDark ? 212 : 238
        );
        const QColor activeTabSurface = withAlpha(
            blendColors(previewTheme.tabSelectedBackground, panelSurface, 0.26),
            previewTheme.isDark ? 228 : 244
        );
        const QColor inactiveTabSurface = withAlpha(
            blendColors(previewTheme.tabBackground, panelSurface, 0.24),
            previewTheme.isDark ? 164 : 214
        );
        const QColor badgeSurface = withAlpha(
            blendColors(accent, panelSurface, previewTheme.isDark ? 0.24 : 0.14),
            previewTheme.isDark ? 236 : 248
        );
        const QColor badgeBorder = withAlpha(
            blendColors(previewTheme.buttonActiveBorder, accent, 0.34),
            224
        );

        const QColor primaryText = previewReadableTextColor(previewTheme, previewSurface, 4.6);
        const QColor headerText = previewReadableTextColor(previewTheme, headerSurface, 4.8);
        const QColor bodyText = previewReadableTextColor(previewTheme, cardSurface, 4.6);
        const QColor badgeText = previewReadableTextColor(previewTheme, badgeSurface, 4.6);
        const QColor mutedText = previewTheme.forceTextColor
            ? primaryText
            : ensureReadableTextColor(
                inactiveTabSurface,
                blendColors(primaryText, shellSurface, previewTheme.isDark ? 0.18 : 0.28),
                4.2
            );

        m_colorPreviewLogoLeft->setPixmap(previewLogoPixmap(20, headerSurface, border, accent, headerText, false));
        m_colorPreviewLogoRight->setPixmap(previewLogoPixmap(20, headerSurface, border, accent, headerText, true));

        auto applyIndexedQss = [](QString style, const std::initializer_list<QString>& values) {
            int index = static_cast<int>(values.size());
            for (auto it = values.end(); it != values.begin();) {
                --it;
                style.replace(QStringLiteral("%") + QString::number(index), *it);
                --index;
            }
            return style;
        };

        m_colorPreviewFrame->setStyleSheet(applyIndexedQss(QStringLiteral(R"(
QFrame#CommandTabColorPreviewFrame {
    background: %1;
    border: 1px solid %2;
    border-radius: 12px;
}
QLabel#CommandTabColorPreviewHeader {
    color: %3;
    font-weight: 700;
    letter-spacing: 0.2px;
}
QWidget#CommandTabColorPreviewRibbonRow {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %4,stop:1 %5);
    border: 1px solid %2;
    border-radius: 10px;
}
QLabel#CommandTabColorPreviewTabActive {
    min-width: 72px;
    padding: 3px 9px;
    border-radius: 8px;
    background: %6;
    border: 1px solid %8;
    color: %9;
    font-weight: 700;
}
QLabel#CommandTabColorPreviewTabInactive {
    min-width: 72px;
    padding: 3px 9px;
    border-radius: 8px;
    background: %7;
    border: 1px solid transparent;
    color: %10;
    font-weight: 600;
}
QLabel#CommandTabColorPreviewAccentBadge {
    min-width: 60px;
    padding: 3px 8px;
    border-radius: 8px;
    background: %11;
    border: 1px solid %12;
    color: %13;
    font-weight: 700;
}
QFrame#CommandTabColorPreviewPanelCard {
    background: %14;
    border: 1px solid %2;
    border-radius: 10px;
}
QLabel#CommandTabColorPreviewPanelTitle {
    padding: 4px 8px;
    border-radius: 8px;
    background: %15;
    border: 1px solid %16;
    color: %17;
    font-weight: 700;
}
QLabel#CommandTabColorPreviewPanelBody {
    color: %18;
}
        )"),
            {
                previewSurface.name(QColor::HexArgb),
                border.name(QColor::HexArgb),
                headerText.name(QColor::HexArgb),
                withAlpha(blendColors(headerSurface, QColor(QStringLiteral("#ffffff")), 0.20), 238).name(QColor::HexArgb),
                withAlpha(blendColors(headerSurface, QColor(QStringLiteral("#0f1724")), 0.12), 220).name(QColor::HexArgb),
                activeTabSurface.name(QColor::HexArgb),
                inactiveTabSurface.name(QColor::HexArgb),
                withAlpha(blendColors(previewTheme.tabSelectedBorder, accent, 0.24), 224).name(QColor::HexArgb),
                primaryText.name(QColor::HexArgb),
                mutedText.name(QColor::HexArgb),
                badgeSurface.name(QColor::HexArgb),
                badgeBorder.name(QColor::HexArgb),
                badgeText.name(QColor::HexArgb),
                cardSurface.name(QColor::HexArgb),
                withAlpha(blendColors(cardSurface, previewTheme.panelFooterBackground, 0.26), 224).name(QColor::HexArgb),
                withAlpha(blendColors(previewTheme.panelFooterBorder, border, 0.34), 216).name(QColor::HexArgb),
                headerText.name(QColor::HexArgb),
                bodyText.name(QColor::HexArgb),
            }));
    }

    void refreshCustomColorControls()
    {
        const bool enabled = m_customMainColorsEnabledCheck != nullptr
            && m_customMainColorsEnabledCheck->isChecked();
        if (m_customMainBackgroundButton != nullptr) {
            m_customMainBackgroundButton->setEnabled(enabled);
        }
        if (m_customMainTextButton != nullptr) {
            m_customMainTextButton->setEnabled(enabled);
        }
        if (m_customRibbonPrimaryButton != nullptr) {
            m_customRibbonPrimaryButton->setEnabled(enabled);
        }
        if (m_customRibbonSecondaryButton != nullptr) {
            m_customRibbonSecondaryButton->setEnabled(enabled);
        }
        if (m_customRibbonAccentButton != nullptr) {
            m_customRibbonAccentButton->setEnabled(enabled);
        }

        const QColor fallbackBackground = m_theme.shellBackground.isValid()
            ? m_theme.shellBackground
            : QColor(QStringLiteral("#1e2329"));
        const QColor fallbackText = m_theme.buttonText.isValid()
            ? m_theme.buttonText
            : QColor(QStringLiteral("#d7dde8"));
        const QString defaultLabel =
            QCoreApplication::translate("CommandTabSettingsDialog", "Theme default");
        refreshColorButton(
            m_customMainBackgroundButton,
            m_customMainBackgroundColor,
            fallbackBackground,
            defaultLabel
        );
        refreshColorButton(
            m_customMainTextButton,
            m_customMainTextColor,
            fallbackText,
            defaultLabel
        );
        refreshColorButton(
            m_customRibbonPrimaryButton,
            m_customRibbonPrimaryColor,
            fallbackBackground,
            defaultLabel
        );
        refreshColorButton(
            m_customRibbonSecondaryButton,
            m_customRibbonSecondaryColor,
            m_theme.panelCardBackground.isValid()
                ? m_theme.panelCardBackground
                : blendColors(fallbackBackground, QColor(QStringLiteral("#ffffff")), 0.30),
            defaultLabel
        );
        refreshColorButton(
            m_customRibbonAccentButton,
            m_customRibbonAccentColor,
            m_theme.tabAccent.isValid()
                ? m_theme.tabAccent
                : QColor(QStringLiteral("#3b82f6")),
            defaultLabel
        );

        const bool vpEnabled = m_viewportColorsEnabledCheck != nullptr
            && m_viewportColorsEnabledCheck->isChecked();
        if (m_viewportBackgroundStyleCombo != nullptr) {
            m_viewportBackgroundStyleCombo->setEnabled(vpEnabled);
        }
        QString viewportStyle = QStringLiteral("linear");
        if (m_viewportBackgroundStyleCombo != nullptr) {
            viewportStyle = m_viewportBackgroundStyleCombo->currentData().toString().trimmed().toLower();
        }

        const bool showMidColor = viewportStyle == QStringLiteral("tricolor")
            || viewportStyle == QStringLiteral("radial")
            || viewportStyle == QStringLiteral("quad");
        const bool showAccentColor = viewportStyle == QStringLiteral("quad");

        if (m_viewportBgTopButton != nullptr) m_viewportBgTopButton->setEnabled(vpEnabled);
        refreshColorButton(m_viewportBgTopButton, m_viewportBgTopColor,
            QColor(QStringLiteral("#192833")), defaultLabel);
        if (m_viewportBgMidButton != nullptr) m_viewportBgMidButton->setEnabled(vpEnabled && showMidColor);
        refreshColorButton(m_viewportBgMidButton, m_viewportBgMidColor,
            QColor(QStringLiteral("#131f29")), defaultLabel);
        if (m_viewportBgBottomButton != nullptr) m_viewportBgBottomButton->setEnabled(vpEnabled);
        refreshColorButton(m_viewportBgBottomButton, m_viewportBgBottomColor,
            QColor(QStringLiteral("#0a0e12")), defaultLabel);
        if (m_viewportBgAccentButton != nullptr) m_viewportBgAccentButton->setEnabled(vpEnabled && showAccentColor);
        refreshColorButton(m_viewportBgAccentButton, m_viewportBgAccentColor,
            QColor(QStringLiteral("#3f5a73")), defaultLabel);

        const bool gridEnabled = m_gridColorEnabledCheck != nullptr
            && m_gridColorEnabledCheck->isChecked();
        if (m_viewportGridColorButton != nullptr) m_viewportGridColorButton->setEnabled(gridEnabled);
        refreshColorButton(m_viewportGridColorButton, m_viewportGridColor,
            QColor(QStringLiteral("#404040")), defaultLabel);
        refreshOptionRowIcons();
        refreshVisualChoiceIcons();
        refreshVisualChoicePreviews();
        refreshColorPreview();
    }

    void exportProfile()
    {
        if (!m_commandHandler) {
            return;
        }
        const QString path = QFileDialog::getSaveFileName(
            this,
            QCoreApplication::translate("CommandTabSettingsDialog", "Export commandtab profile"),
            QDir::homePath() + QStringLiteral("/CommandTabStructure.json"),
            QCoreApplication::translate("CommandTabSettingsDialog", "CommandTab Structure (*.json)")
        );
        if (path.isEmpty()) {
            return;
        }
        m_commandHandler(
            QStringLiteral("__commandtab_design_export__:%1").arg(encodeCommandTabCommandArgument(path))
        );
    }

    void importProfile()
    {
        if (!m_commandHandler) {
            return;
        }
        const QString path = QFileDialog::getOpenFileName(
            this,
            QCoreApplication::translate("CommandTabSettingsDialog", "Import commandtab profile"),
            QDir::homePath(),
            QCoreApplication::translate("CommandTabSettingsDialog", "CommandTab Structure (*.json)")
        );
        if (path.isEmpty()) {
            return;
        }
        m_commandHandler(
            QStringLiteral("__commandtab_design_import__:%1").arg(encodeCommandTabCommandArgument(path))
        );
    }

    void submit(const QString& prefix)
    {
        if (!m_commandHandler) {
            return;
        }

        QJsonObject payload;
        payload.insert(QStringLiteral("preferNativeCommandTab"), m_preferNativeCommandTabCheck->isChecked());
        payload.insert(QStringLiteral("nativeCommandTabWarmup"), m_nativeCommandTabWarmupCheck->isChecked());
        payload.insert(QStringLiteral("modernCommandTabStyleEnabled"), m_modernCommandTabStyleCheck->isChecked());
        payload.insert(QStringLiteral("hideMenuBarInNativeMode"), m_hideMenuBarInNativeModeCheck->isChecked());
        payload.insert(QStringLiteral("ribbonAutoHide"), m_ribbonAutoHideCheck->isChecked());
        payload.insert(QStringLiteral("ribbonAutoHideDelayMs"), m_ribbonAutoHideDelaySpinner->value());
        payload.insert(QStringLiteral("ribbonHoverTab"), m_ribbonHoverTabCheck->isChecked());
        payload.insert(QStringLiteral("tabClickPopupMode"), m_tabClickPopupModeCheck->isChecked());
        payload.insert(QStringLiteral("nativeThemeMode"), m_nativeThemeModeCombo->currentData().toString());
        payload.insert(QStringLiteral("compactPanelLayout"), m_compactPanelLayoutCheck->isChecked());
        payload.insert(QStringLiteral("panelDropdownModeEnabled"), m_panelDropdownModeCheck->isChecked());
        payload.insert(
            QStringLiteral("panelDropdownPrimaryRecent"),
            m_panelDropdownPrimaryRecentCheck->isChecked()
        );
        payload.insert(
            QStringLiteral("panelDropdownRecentToolCount"),
            m_panelDropdownRecentToolCountSpin->value()
        );
        payload.insert(
            QStringLiteral("panelDropdownPopupColumns"),
            m_panelDropdownPopupColumnsSpin->value()
        );
        payload.insert(
            QStringLiteral("panelDropdownPopupIconSize"),
            m_panelDropdownPopupIconSizeSpin->value()
        );
        payload.insert(QStringLiteral("headerScalePercent"), m_headerScalePercentSpin->value());
        payload.insert(QStringLiteral("commandtabScalePercent"), m_commandtabScalePercentSpin->value());
        // Backward compatibility for older payload readers.
        payload.insert(QStringLiteral("displayScalePercent"), m_commandtabScalePercentSpin->value());
        payload.insert(
            QStringLiteral("panelDropdownPopupShowText"),
            m_panelDropdownPopupShowTextCheck->isChecked()
        );
        payload.insert(QStringLiteral("compactPanelSpacing"), m_compactPanelSpacingSpin->value());
        payload.insert(QStringLiteral("compactButtonPadding"), m_compactButtonPaddingSpin->value());
        payload.insert(QStringLiteral("showIconTextSmall"), m_showIconTextSmallCheck->isChecked());
        payload.insert(QStringLiteral("showIconTextMedium"), m_showIconTextMediumCheck->isChecked());
        payload.insert(QStringLiteral("showIconTextLarge"), m_showIconTextLargeCheck->isChecked());
        payload.insert(QStringLiteral("iconOnlySizeSmall"), m_iconOnlySizeSmallSpin->value());
        payload.insert(QStringLiteral("iconOnlySizeMedium"), m_iconOnlySizeMediumSpin->value());
        payload.insert(QStringLiteral("iconOnlySizeLarge"), m_iconOnlySizeLargeSpin->value());
        payload.insert(QStringLiteral("showSketcherGrid"), m_showSketcherGridCheck->isChecked());
        payload.insert(QStringLiteral("snapSketcherGrid"), m_snapSketcherGridCheck->isChecked());
        payload.insert(
            QStringLiteral("customMainColorsEnabled"),
            m_customMainColorsEnabledCheck->isChecked()
        );
        payload.insert(
            QStringLiteral("customMainBackgroundColor"),
            colorHexOrEmpty(m_customMainBackgroundColor)
        );
        payload.insert(
            QStringLiteral("customMainTextColor"),
            colorHexOrEmpty(m_customMainTextColor)
        );
        payload.insert(
            QStringLiteral("customRibbonPrimaryColor"),
            colorHexOrEmpty(m_customRibbonPrimaryColor)
        );
        payload.insert(
            QStringLiteral("customRibbonSecondaryColor"),
            colorHexOrEmpty(m_customRibbonSecondaryColor)
        );
        payload.insert(
            QStringLiteral("customRibbonAccentColor"),
            colorHexOrEmpty(m_customRibbonAccentColor)
        );
        payload.insert(QStringLiteral("viewportColorsEnabled"),
            m_viewportColorsEnabledCheck->isChecked());
        payload.insert(
            QStringLiteral("viewportBackgroundStyle"),
            m_viewportBackgroundStyleCombo->currentData().toString()
        );
        payload.insert(QStringLiteral("viewportBgTopColor"),
            colorHexOrEmpty(m_viewportBgTopColor));
        payload.insert(QStringLiteral("viewportBgMidColor"),
            colorHexOrEmpty(m_viewportBgMidColor));
        payload.insert(QStringLiteral("viewportBgBottomColor"),
            colorHexOrEmpty(m_viewportBgBottomColor));
        payload.insert(QStringLiteral("viewportBgAccentColor"),
            colorHexOrEmpty(m_viewportBgAccentColor));
        payload.insert(QStringLiteral("gridColorEnabled"),
            m_gridColorEnabledCheck->isChecked());
        payload.insert(QStringLiteral("viewportGridColor"),
            colorHexOrEmpty(m_viewportGridColor));
        const QByteArray json = QJsonDocument(payload).toJson(QJsonDocument::Compact);
        const QString encoded = QString::fromLatin1(
            json.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)
        );
        m_commandHandler(QStringLiteral("%1:%2").arg(prefix, encoded));
    }

    std::function<void(const QString&)> m_commandHandler;
    QCheckBox* m_preferNativeCommandTabCheck = nullptr;
    QCheckBox* m_nativeCommandTabWarmupCheck = nullptr;
    QCheckBox* m_modernCommandTabStyleCheck = nullptr;
    QCheckBox* m_hideMenuBarInNativeModeCheck = nullptr;
    QCheckBox* m_ribbonAutoHideCheck = nullptr;
    QSpinBox* m_ribbonAutoHideDelaySpinner = nullptr;
    QCheckBox* m_ribbonHoverTabCheck = nullptr;
    QCheckBox* m_tabClickPopupModeCheck = nullptr;
    QCheckBox* m_compactPanelLayoutCheck = nullptr;
    QCheckBox* m_panelDropdownModeCheck = nullptr;
    QCheckBox* m_panelDropdownPrimaryRecentCheck = nullptr;
    QSpinBox* m_panelDropdownRecentToolCountSpin = nullptr;
    QSpinBox* m_panelDropdownPopupColumnsSpin = nullptr;
    QSpinBox* m_panelDropdownPopupIconSizeSpin = nullptr;
    QSpinBox* m_headerScalePercentSpin = nullptr;
    QSpinBox* m_commandtabScalePercentSpin = nullptr;
    QCheckBox* m_panelDropdownPopupShowTextCheck = nullptr;
    QSpinBox* m_compactPanelSpacingSpin = nullptr;
    QSpinBox* m_compactButtonPaddingSpin = nullptr;
    QCheckBox* m_showIconTextSmallCheck = nullptr;
    QCheckBox* m_showIconTextMediumCheck = nullptr;
    QCheckBox* m_showIconTextLargeCheck = nullptr;
    QSpinBox* m_iconOnlySizeSmallSpin = nullptr;
    QSpinBox* m_iconOnlySizeMediumSpin = nullptr;
    QSpinBox* m_iconOnlySizeLargeSpin = nullptr;
    QCheckBox* m_showSketcherGridCheck = nullptr;
    QCheckBox* m_snapSketcherGridCheck = nullptr;
    QComboBox* m_nativeThemeModeCombo = nullptr;
    QLabel* m_nativeThemePreviewLabel = nullptr;
    QTabWidget* m_tabs = nullptr;
    QCheckBox* m_customMainColorsEnabledCheck = nullptr;
    QPushButton* m_customMainBackgroundButton = nullptr;
    QPushButton* m_customMainTextButton = nullptr;
    QPushButton* m_customRibbonPrimaryButton = nullptr;
    QPushButton* m_customRibbonSecondaryButton = nullptr;
    QPushButton* m_customRibbonAccentButton = nullptr;
    QFrame* m_colorPreviewFrame = nullptr;
    QWidget* m_colorPreviewRibbonRow = nullptr;
    QLabel* m_colorPreviewLogoLeft = nullptr;
    QLabel* m_colorPreviewTabActive = nullptr;
    QLabel* m_colorPreviewTabInactive = nullptr;
    QLabel* m_colorPreviewAccentBadge = nullptr;
    QLabel* m_colorPreviewLogoRight = nullptr;
    QFrame* m_colorPreviewPanelCard = nullptr;
    QLabel* m_colorPreviewPanelTitle = nullptr;
    QLabel* m_colorPreviewPanelBody = nullptr;
    QColor m_customMainBackgroundColor;
    QColor m_customMainTextColor;
    QColor m_customRibbonPrimaryColor;
    QColor m_customRibbonSecondaryColor;
    QColor m_customRibbonAccentColor;
    QCheckBox* m_viewportColorsEnabledCheck = nullptr;
    QComboBox* m_viewportBackgroundStyleCombo = nullptr;
    QLabel* m_viewportStylePreviewLabel = nullptr;
    QPushButton* m_viewportBgTopButton = nullptr;
    QColor m_viewportBgTopColor;
    QPushButton* m_viewportBgMidButton = nullptr;
    QColor m_viewportBgMidColor;
    QPushButton* m_viewportBgBottomButton = nullptr;
    QColor m_viewportBgBottomColor;
    QPushButton* m_viewportBgAccentButton = nullptr;
    QColor m_viewportBgAccentColor;
    QCheckBox* m_gridColorEnabledCheck = nullptr;
    QPushButton* m_viewportGridColorButton = nullptr;
    QColor m_viewportGridColor;
    CommandTabModel::CommandTabTheme m_theme;
};

struct CommandTabMetadataEntry
{
    QString text;
    QString iconPath;
};

struct CommandTabMetadataCache
{
    QHash<QString, CommandTabMetadataEntry> commands;
    QHash<QString, QString> workbenchTitles;
    QHash<QString, QString> workbenchIcons;
    QHash<QString, QString> panelTitles;
};

struct CommandTabCommandOverrides
{
    QHash<QString, QJsonObject> specialCommandInfoUpdates;
    QHash<QString, QString> specialPixmaps;
    QHash<QString, QString> customCommandPixmapFiles;
};

QHash<QString, CommandTabCommandOverrides> g_commandOverridesCache;

bool loadJsonObjectFromFile(const QString& path, QJsonObject* object, const QString& label)
{
    if (object == nullptr) {
        setLastError(QStringLiteral("%1 output is null").arg(label));
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setLastError(QStringLiteral("Unable to open %1: %2").arg(label, path));
        return false;
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setLastError(QStringLiteral("Invalid %1 JSON: %2").arg(label, parseError.errorString()));
        return false;
    }

    *object = document.object();
    return true;
}

bool writeJsonObjectToFile(const QString& path, const QJsonObject& object, const QString& label)
{
    QFile outputFile(path);
    const QFileInfo outputInfo(outputFile);
    QDir().mkpath(outputInfo.absolutePath());
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setLastError(QStringLiteral("Unable to write %1: %2").arg(label, path));
        return false;
    }
    outputFile.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    outputFile.close();
    return true;
}

bool copyFileReplacing(const QString& sourcePath, const QString& targetPath, const QString& label)
{
    const QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists() || !sourceInfo.isFile()) {
        setLastError(QStringLiteral("Missing %1 source file: %2").arg(label, sourcePath));
        return false;
    }

    const QFileInfo targetInfo(targetPath);
    QDir().mkpath(targetInfo.absolutePath());
    if (QFile::exists(targetPath) && !QFile::remove(targetPath)) {
        setLastError(QStringLiteral("Unable to replace %1 target file: %2").arg(label, targetPath));
        return false;
    }
    if (!QFile::copy(sourcePath, targetPath)) {
        setLastError(QStringLiteral("Unable to copy %1 file to: %2").arg(label, targetPath));
        return false;
    }
    return true;
}

QStringList sortedUniqueStrings(const QSet<QString>& values)
{
    QStringList orderedValues = values.values();
    orderedValues.removeAll(QString());
    std::sort(orderedValues.begin(), orderedValues.end(), [](const QString& left, const QString& right) {
        return left.compare(right, Qt::CaseInsensitive) < 0;
    });
    orderedValues.removeDuplicates();
    return orderedValues;
}

QJsonArray jsonArrayFromStrings(const QStringList& values)
{
    QJsonArray array;
    for (const auto& value : values) {
        if (!value.isEmpty()) {
            array.append(value);
        }
    }
    return array;
}

QJsonArray jsonArrayFromStrings(const QSet<QString>& values)
{
    return jsonArrayFromStrings(sortedUniqueStrings(values));
}

QString scopedIgnoredToolbarKey(const QString& workbenchId, const QString& panelId)
{
    const QString normalizedWorkbenchId = workbenchId.trimmed();
    const QString normalizedPanelId = panelId.trimmed();
    if (normalizedWorkbenchId.isEmpty() || normalizedPanelId.isEmpty()) {
        return QString();
    }
    return normalizedWorkbenchId + QStringLiteral("::") + normalizedPanelId;
}

QSet<QString> jsonStringSet(const QJsonArray& array)
{
    QSet<QString> values;
    for (const auto& item : array) {
        const QString value = item.toString().trimmed();
        if (!value.isEmpty()) {
            values.insert(value);
        }
    }
    return values;
}

bool applyCustomizationToStructure(
    const QByteArray& customizationJson,
    const QString& structurePath
)
{
    QJsonParseError parseError;
    const auto payloadDocument = QJsonDocument::fromJson(customizationJson, &parseError);
    if (parseError.error != QJsonParseError::NoError || !payloadDocument.isObject()) {
        setLastError(QStringLiteral("Invalid customization JSON: %1").arg(parseError.errorString()));
        return false;
    }

    QJsonObject structureRoot;
    if (!loadJsonObjectFromFile(structurePath, &structureRoot, QStringLiteral("editable commandtab structure"))) {
        return false;
    }

    const QJsonObject payload = payloadDocument.object();

    QJsonArray quickAccessCommands;
    for (const auto& commandValue : payload.value(QStringLiteral("quickAccess")).toArray()) {
        const QString commandId = commandValue.toString().trimmed();
        if (!commandId.isEmpty()) {
            quickAccessCommands.append(commandId);
        }
    }
    structureRoot.insert(QStringLiteral("quickAccessCommands"), quickAccessCommands);

    const QSet<QString> existingIgnoredToolbars = jsonStringSet(
        structureRoot.value(QStringLiteral("ignoredToolbars")).toArray()
    );
    QSet<QString> representedPanelIds;
    QSet<QString> representedScopedPanels;
    QSet<QString> hiddenPanels;
    for (const auto& panelValue : payload.value(QStringLiteral("panelVisibility")).toArray()) {
        if (!panelValue.isObject()) {
            continue;
        }
        const auto panelObject = panelValue.toObject();
        const QString panelId = panelObject.value(QStringLiteral("id")).toString().trimmed();
        const QString workbenchId = panelObject.value(QStringLiteral("workbenchId")).toString().trimmed();
        if (panelId.isEmpty()) {
            continue;
        }
        representedPanelIds.insert(panelId);
        const QString scopedPanelKey = scopedIgnoredToolbarKey(workbenchId, panelId);
        if (!scopedPanelKey.isEmpty()) {
            representedScopedPanels.insert(scopedPanelKey);
        }
        if (!panelObject.value(QStringLiteral("visible")).toBool(true)) {
            hiddenPanels.insert(scopedPanelKey.isEmpty() ? panelId : scopedPanelKey);
        }
    }
    QSet<QString> preservedIgnoredToolbars;
    for (const auto& item : existingIgnoredToolbars) {
        if (item.contains(QStringLiteral("::"))) {
            if (!representedScopedPanels.contains(item)) {
                preservedIgnoredToolbars.insert(item);
            }
            continue;
        }
        if (!representedPanelIds.contains(item)) {
            preservedIgnoredToolbars.insert(item);
        }
    }
    QSet<QString> mergedIgnoredToolbars = preservedIgnoredToolbars;
    mergedIgnoredToolbars.unite(hiddenPanels);
    structureRoot.insert(
        QStringLiteral("ignoredToolbars"),
        jsonArrayFromStrings(mergedIgnoredToolbars)
    );

    QSet<QString> hiddenWorkbenches = jsonStringSet(payload.value(QStringLiteral("hiddenWorkbenches")).toArray());
    if (hiddenWorkbenches.isEmpty()) {
        for (const auto& workbenchValue : payload.value(QStringLiteral("workbenchVisibility")).toArray()) {
            if (!workbenchValue.isObject()) {
                continue;
            }
            const auto workbenchObject = workbenchValue.toObject();
            const QString workbenchId = workbenchObject.value(QStringLiteral("id")).toString().trimmed();
            if (workbenchId.isEmpty()) {
                continue;
            }
            if (!workbenchObject.value(QStringLiteral("visible")).toBool(true)) {
                hiddenWorkbenches.insert(workbenchId);
            }
        }
    }
    structureRoot.insert(QStringLiteral("ignoredWorkbenches"), jsonArrayFromStrings(hiddenWorkbenches));

    const QJsonObject existingDropdownButtons = structureRoot.value(QStringLiteral("dropdownButtons")).toObject();
    QJsonObject dropdownButtons;
    for (const auto& dropdownValue : payload.value(QStringLiteral("dropdowns")).toArray()) {
        if (!dropdownValue.isObject()) {
            continue;
        }
        const auto dropdownObject = dropdownValue.toObject();
        const QString dropdownId = dropdownObject.value(QStringLiteral("id")).toString().trimmed();
        if (dropdownId.isEmpty()) {
            continue;
        }

        QHash<QString, QString> existingDropdownSources;
        const auto existingItems = existingDropdownButtons.value(dropdownId).toArray();
        for (const auto& itemValue : existingItems) {
            if (!itemValue.isArray()) {
                continue;
            }
            const auto itemArray = itemValue.toArray();
            if (itemArray.isEmpty()) {
                continue;
            }
            const QString commandId = itemArray.at(0).toString().trimmed();
            const QString sourceWorkbenchId = itemArray.size() > 1 ? itemArray.at(1).toString().trimmed() : QString();
            if (!commandId.isEmpty()) {
                existingDropdownSources.insert(commandId, sourceWorkbenchId);
            }
        }

        QJsonArray dropdownItems;
        for (const auto& menuCommandValue : dropdownObject.value(QStringLiteral("commands")).toArray()) {
            if (!menuCommandValue.isObject()) {
                continue;
            }
            const auto menuCommandObject = menuCommandValue.toObject();
            const QString menuCommandId = menuCommandObject.value(QStringLiteral("id")).toString().trimmed();
            if (menuCommandId.isEmpty()) {
                continue;
            }
            QString sourceWorkbenchId =
                menuCommandObject.value(QStringLiteral("sourceWorkbenchId")).toString().trimmed();
            if (sourceWorkbenchId.isEmpty()) {
                sourceWorkbenchId = existingDropdownSources.value(menuCommandId);
            }
            if (sourceWorkbenchId.isEmpty()) {
                sourceWorkbenchId = QStringLiteral("General");
            }
            QJsonArray dropdownItem;
            dropdownItem.append(menuCommandId);
            dropdownItem.append(sourceWorkbenchId);
            dropdownItems.append(dropdownItem);
        }
        dropdownButtons.insert(dropdownId, dropdownItems);
    }
    structureRoot.insert(QStringLiteral("dropdownButtons"), dropdownButtons);

    QHash<QString, QJsonObject> payloadWorkbenches;
    for (const auto& workbenchValue : payload.value(QStringLiteral("workbenches")).toArray()) {
        if (!workbenchValue.isObject()) {
            continue;
        }
        const auto workbenchObject = workbenchValue.toObject();
        const QString workbenchId = workbenchObject.value(QStringLiteral("id")).toString().trimmed();
        if (!workbenchId.isEmpty()) {
            payloadWorkbenches.insert(workbenchId, workbenchObject);
        }
    }

    QJsonObject structureWorkbenches = structureRoot.value(QStringLiteral("workbenches")).toObject();
    QJsonObject structureCustomToolbars = structureRoot.value(QStringLiteral("customToolbars")).toObject();
    QJsonObject structureNewPanels = structureRoot.value(QStringLiteral("newPanels")).toObject();
    QHash<QString, QJsonObject> rebuiltNewPanelsBySource;

    for (auto workbenchIt = structureWorkbenches.begin(); workbenchIt != structureWorkbenches.end(); ++workbenchIt) {
        const QString workbenchId = workbenchIt.key();
        if (!workbenchIt.value().isObject()) {
            continue;
        }
        const auto payloadWorkbenchIt = payloadWorkbenches.constFind(workbenchId);
        if (payloadWorkbenchIt == payloadWorkbenches.constEnd()) {
            continue;
        }

        QJsonObject workbenchData = workbenchIt.value().toObject();
        const QJsonObject payloadWorkbench = payloadWorkbenchIt.value();
        const QJsonObject existingToolbars = workbenchData.value(QStringLiteral("toolbars")).toObject();
        const QJsonObject existingCustomPanels = structureCustomToolbars.value(workbenchId).toObject();

        QJsonObject reorderedToolbars;
        QJsonObject rebuiltCustomPanels;
        QSet<QString> seenStandardPanels;
        QJsonArray orderedPanelIds;

        for (const auto& panelValue : payloadWorkbench.value(QStringLiteral("panels")).toArray()) {
            if (!panelValue.isObject()) {
                continue;
            }
            const auto panelObject = panelValue.toObject();
            const QString panelId = panelObject.value(QStringLiteral("id")).toString().trimmed();
            if (panelId.isEmpty()) {
                continue;
            }
            orderedPanelIds.append(panelId);

            QString sourceType = panelObject.value(QStringLiteral("sourceType")).toString(QStringLiteral("toolbar")).trimmed().toLower();
            if (
                sourceType != QStringLiteral("toolbar")
                && sourceType != QStringLiteral("custom")
                && sourceType != QStringLiteral("new")
            ) {
                sourceType = QStringLiteral("toolbar");
            }

            QString sourceWorkbenchId = panelObject.value(QStringLiteral("sourceWorkbenchId")).toString().trimmed();
            if (sourceWorkbenchId.isEmpty()) {
                sourceWorkbenchId = sourceType == QStringLiteral("new") ? workbenchId : workbenchId;
            }

            struct PendingCommand
            {
                QString id;
                QString type;
                QJsonObject payload;
            };

            QVector<PendingCommand> pendingCommands;
            int separatorIndex = 0;
            for (const auto& commandValue : panelObject.value(QStringLiteral("commands")).toArray()) {
                if (!commandValue.isObject()) {
                    continue;
                }
                const auto commandObject = commandValue.toObject();
                const QString commandType =
                    commandObject.value(QStringLiteral("type")).toString(QStringLiteral("command")).trimmed().toLower();
                if (commandType == QStringLiteral("separator")) {
                    QString commandId = commandObject.value(QStringLiteral("id")).toString().trimmed();
                    if (commandId.isEmpty()) {
                        commandId = QStringLiteral("%1_separator_%2").arg(separatorIndex++).arg(workbenchId);
                    }
                    pendingCommands.push_back({commandId, commandType, QJsonObject()});
                    continue;
                }

                const QString commandId = commandObject.value(QStringLiteral("id")).toString().trimmed();
                if (commandId.isEmpty()) {
                    continue;
                }
                pendingCommands.push_back({commandId, commandType, commandObject});
            }

            if (sourceType == QStringLiteral("custom")) {
                const QJsonObject existingCustomCommands =
                    existingCustomPanels.value(panelId).toObject().value(QStringLiteral("commands")).toObject();
                QJsonObject customCommands;
                for (const auto& command : pendingCommands) {
                    if (command.type == QStringLiteral("separator")) {
                        continue;
                    }
                    QString sourceToolbarTitle =
                        command.payload.value(QStringLiteral("sourceToolbarTitle")).toString().trimmed();
                    if (sourceToolbarTitle.isEmpty()) {
                        sourceToolbarTitle = existingCustomCommands.value(command.id).toString().trimmed();
                    }
                    customCommands.insert(command.id, sourceToolbarTitle);
                }
                QJsonObject customPanelObject;
                customPanelObject.insert(QStringLiteral("commands"), customCommands);
                rebuiltCustomPanels.insert(panelId, customPanelObject);
                continue;
            }

            if (sourceType == QStringLiteral("new")) {
                QJsonArray newPanelCommands;
                for (const auto& command : pendingCommands) {
                    QString commandSourceWorkbenchId =
                        command.payload.value(QStringLiteral("sourceWorkbenchId")).toString().trimmed();
                    if (commandSourceWorkbenchId.isEmpty()) {
                        commandSourceWorkbenchId = sourceWorkbenchId;
                    }
                    if (commandSourceWorkbenchId.isEmpty()) {
                        commandSourceWorkbenchId = QStringLiteral("General");
                    }
                    QJsonArray commandItem;
                    commandItem.append(command.id);
                    commandItem.append(commandSourceWorkbenchId);
                    newPanelCommands.append(commandItem);
                }
                QJsonObject sourcePanels = rebuiltNewPanelsBySource.value(sourceWorkbenchId);
                sourcePanels.insert(panelId, newPanelCommands);
                rebuiltNewPanelsBySource.insert(sourceWorkbenchId, sourcePanels);
                continue;
            }

            QJsonObject panelData = existingToolbars.value(panelId).toObject();
            QJsonObject reorderedCommands = panelData.value(QStringLiteral("commands")).toObject();
            QJsonArray commandOrder;
            seenStandardPanels.insert(panelId);

            for (const auto& command : pendingCommands) {
                commandOrder.append(command.id);
                if (command.type == QStringLiteral("separator")) {
                    if (!reorderedCommands.contains(command.id)) {
                        reorderedCommands.insert(command.id, QJsonObject());
                    }
                    continue;
                }

                QJsonObject commandData = reorderedCommands.value(command.id).toObject();
                commandData.remove(QStringLiteral("textEnabled"));
                commandData.remove(QStringLiteral("textVisible"));
                QString size = command.payload.value(QStringLiteral("size")).toString(
                    commandData.value(QStringLiteral("size")).toString(QStringLiteral("small"))
                ).trimmed().toLower();
                if (
                    size != QStringLiteral("small")
                    && size != QStringLiteral("medium")
                    && size != QStringLiteral("large")
                ) {
                    size = QStringLiteral("small");
                }
                commandData.insert(QStringLiteral("size"), size);
                if (command.payload.contains(QStringLiteral("text"))) {
                    commandData.insert(
                        QStringLiteral("text"),
                        command.payload.value(QStringLiteral("text")).toString()
                    );
                }
                reorderedCommands.insert(command.id, commandData);
            }

            panelData.insert(QStringLiteral("order"), commandOrder);
            panelData.insert(QStringLiteral("commands"), reorderedCommands);
            panelData.insert(
                QStringLiteral("title"),
                panelObject.value(QStringLiteral("title")).toString(
                    panelData.value(QStringLiteral("title")).toString(panelId)
                )
            );
            panelData.insert(QStringLiteral("Enabled"), true);
            reorderedToolbars.insert(panelId, panelData);
        }

        for (auto toolbarIt = existingToolbars.begin(); toolbarIt != existingToolbars.end(); ++toolbarIt) {
            const QString panelId = toolbarIt.key();
            if (panelId == QStringLiteral("order") || seenStandardPanels.contains(panelId)) {
                continue;
            }
            if (toolbarIt.value().isObject()) {
                QJsonObject hiddenPanel = toolbarIt.value().toObject();
                hiddenPanel.insert(QStringLiteral("Enabled"), false);
                reorderedToolbars.insert(panelId, hiddenPanel);
            } else {
                reorderedToolbars.insert(panelId, toolbarIt.value());
            }
        }

        reorderedToolbars.insert(QStringLiteral("order"), orderedPanelIds);
        workbenchData.insert(QStringLiteral("toolbars"), reorderedToolbars);
        workbenchIt.value() = workbenchData;
        structureCustomToolbars.insert(workbenchId, rebuiltCustomPanels);
        structureNewPanels.insert(workbenchId, rebuiltNewPanelsBySource.value(workbenchId));
        if (rebuiltNewPanelsBySource.contains(QStringLiteral("Global")) || structureNewPanels.contains(QStringLiteral("Global"))) {
            structureNewPanels.insert(QStringLiteral("Global"), rebuiltNewPanelsBySource.value(QStringLiteral("Global")));
        }
    }

    structureRoot.insert(QStringLiteral("workbenches"), structureWorkbenches);
    structureRoot.insert(QStringLiteral("customToolbars"), structureCustomToolbars);
    structureRoot.insert(QStringLiteral("newPanels"), structureNewPanels);

    return writeJsonObjectToFile(structurePath, structureRoot, QStringLiteral("editable commandtab structure"));
}

bool isSeparatorCommand(const QString& commandId)
{
    return commandId.contains(QStringLiteral("_separator_")) || commandId.endsWith(QStringLiteral("_separator"));
}

QStringList orderedToolbarIds(const QJsonObject& toolbarsObject);
QStringList orderedCommandIds(const QJsonObject& panelObject);
QString normalizeActionCandidate(QString value);
bool looksLikeCommandId(const QString& value);
bool looksLikeTechnicalCommandText(const QString& value, const QString& commandId);
bool isLikelyLowResolutionIconFile(const QString& iconPath, int minEdgePixels);
bool isIgnoredToolbar(
    const QJsonObject& structureRoot,
    const QString& workbenchId,
    const QString& panelId,
    const QJsonObject& panelObject
);
QString panelTitleFromId(const QString& panelId, const QString& suffix);
QString exportActionIconPath(const QString& commandId, const QIcon& icon, const QString& iconDirectoryPath);
void collectDropdownMetadataRequirements(
    const QJsonObject& structureRoot,
    const QString& commandId,
    QSet<QString>* commandIds,
    QHash<QString, QJsonObject>* commandDefinitions
);
bool workbenchHasEnabledPanels(
    const QJsonObject& structureRoot,
    const QString& workbenchId,
    const QJsonObject& workbenchObject
);

QString addonRootFromStructurePath(const QString& structurePath)
{
    auto hasAddonMarkers = [](const QString& candidatePath) {
        const QString normalizedPath = QDir::cleanPath(candidatePath.trimmed());
        if (normalizedPath.isEmpty()) {
            return false;
        }
        const QDir rootDir(normalizedPath);
        const QString overridesPath = rootDir.absoluteFilePath(
            QStringLiteral("freecad_commandtab/native/command_overrides.json")
        );
        const QString structureTemplatePath = rootDir.absoluteFilePath(
            QStringLiteral("CreateStructure.txt")
        );
        return QFileInfo::exists(overridesPath) && QFileInfo::exists(structureTemplatePath);
    };

    auto canonicalOrCleanPath = [](const QString& rawPath) {
        const QFileInfo info(rawPath);
        if (info.exists()) {
            const QString canonical = info.canonicalFilePath();
            if (!canonical.isEmpty()) {
                return canonical;
            }
        }
        return QDir::cleanPath(rawPath);
    };

    auto discoverAddonRoot = [&](const QString& startPath) {
        QString current = canonicalOrCleanPath(startPath);
        for (int depth = 0; depth < 8 && !current.isEmpty(); ++depth) {
            if (hasAddonMarkers(current)) {
                return current;
            }
            const QDir currentDir(current);
            const QString parent = canonicalOrCleanPath(currentDir.absoluteFilePath(QStringLiteral("..")));
            if (parent.isEmpty() || parent == current) {
                break;
            }
            current = parent;
        }
        return QString();
    };

    const QString structureDirectory = QFileInfo(structurePath).absolutePath();
    const QString fromStructure = discoverAddonRoot(structureDirectory);
    if (!fromStructure.isEmpty()) {
        return fromStructure;
    }

    // In normal FreeCAD runtime, addon icons are registered under the `icons:`
    // search paths. Use those paths to recover the addon root when the structure
    // file lives in user-state directories.
    for (const auto& iconSearchPath : QDir::searchPaths(QStringLiteral("icons"))) {
        QString basePath = canonicalOrCleanPath(iconSearchPath);
        if (basePath.isEmpty()) {
            continue;
        }

        const QString fromIconPath = discoverAddonRoot(basePath);
        if (!fromIconPath.isEmpty()) {
            return fromIconPath;
        }

        const QString viaResources = discoverAddonRoot(
            QDir(basePath).absoluteFilePath(QStringLiteral("../.."))
        );
        if (!viaResources.isEmpty()) {
            return viaResources;
        }
    }

    return structureDirectory;
}

QStringList bundledIconExtensions()
{
    return {
        QStringLiteral(".svg"),
        QStringLiteral(".png"),
        QStringLiteral(".xpm"),
    };
}

QString directlyLoadableIconSource(const QString& iconValue)
{
    const QString normalized = iconValue.trimmed();
    if (normalized.isEmpty()) {
        return QString();
    }

    if (normalized.startsWith(QStringLiteral(":/"))) {
        const QIcon icon(normalized);
        if (!icon.isNull()) {
            return normalized;
        }
    }
    if (normalized.startsWith(QStringLiteral("icons:"))) {
        const QIcon icon(normalized);
        if (!icon.isNull()) {
            return normalized;
        }
    }

    const QFileInfo iconInfo(normalized);
    if (iconInfo.exists()) {
        return iconInfo.absoluteFilePath();
    }

    return QString();
}

bool isDirectFileOrResourceIconSource(const QString& iconSource)
{
    const QString normalized = iconSource.trimmed();
    if (normalized.isEmpty()) {
        return false;
    }
    if (normalized.startsWith(QStringLiteral(":/"))) {
        return true;
    }
    if (normalized.startsWith(QStringLiteral("icons:"))) {
        const QIcon icon(normalized);
        if (!icon.isNull()) {
            return true;
        }
    }
    return QFileInfo(normalized).exists();
}

QJsonObject parseJsonObjectText(const QString& value)
{
    if (value.trimmed().isEmpty()) {
        return QJsonObject();
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(value.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return QJsonObject();
    }
    return document.object();
}

const char* duplicateUtf8(const QByteArray& payload)
{
    char* value = static_cast<char*>(std::malloc(static_cast<size_t>(payload.size()) + 1));
    if (value == nullptr) {
        return nullptr;
    }
    std::memcpy(value, payload.constData(), static_cast<size_t>(payload.size()));
    value[payload.size()] = '\0';
    return value;
}

QStringList commandIconCandidates(const QString& commandId)
{
    QStringList candidates;
    const QString normalized = commandId.trimmed();
    if (normalized.isEmpty()) {
        return candidates;
    }

    auto appendUnique = [&candidates](const QString& value) {
        const QString trimmed = value.trimmed();
        if (!trimmed.isEmpty() && !candidates.contains(trimmed)) {
            candidates.push_back(trimmed);
        }
    };
    auto appendRichVariants = [&appendUnique](const QString& value) {
        const QString token = value.trimmed();
        if (token.isEmpty()) {
            return;
        }
        appendUnique(token);

        QString compact = token;
        compact.replace(QLatin1Char('\\'), QLatin1Char('/'));
        compact = compact.simplified();
        appendUnique(compact);

        QString withUnderscores = compact;
        withUnderscores.replace(QLatin1Char(' '), QLatin1Char('_'));
        appendUnique(withUnderscores);

        QString withDashes = withUnderscores;
        withDashes.replace(QLatin1Char('_'), QLatin1Char('-'));
        appendUnique(withDashes);

        appendUnique(compact.toLower());
        appendUnique(withUnderscores.toLower());
        appendUnique(withDashes.toLower());

        const QString baseName = QFileInfo(token).completeBaseName();
        if (!baseName.isEmpty() && baseName != token) {
            appendUnique(baseName);
            appendUnique(baseName.toLower());
        }
    };

    appendRichVariants(normalized);
    if (normalized.endsWith(QStringLiteral("_ddb")) && normalized.size() > 4) {
        appendRichVariants(normalized.left(normalized.size() - 4));
    }

    const qsizetype parentSeparatorIndex = normalized.indexOf(QStringLiteral(", "));
    if (parentSeparatorIndex > 0) {
        const QString parentCommand = normalized.left(parentSeparatorIndex).trimmed();
        if (!parentCommand.isEmpty()) {
            appendRichVariants(parentCommand);
            if (parentCommand.endsWith(QStringLiteral("_ddb")) && parentCommand.size() > 4) {
                appendRichVariants(parentCommand.left(parentCommand.size() - 4));
            }
        }
    }

    const QString normalizedLower = normalized.toLower();
    if (
        normalizedLower == QStringLiteral("createbom_overall")
        || normalizedLower == QStringLiteral("assembly_createbom")
    ) {
        appendRichVariants(QStringLiteral("Assembly_BillOfMaterials.svg"));
        appendRichVariants(QStringLiteral("Assembly_CreateBom"));
        appendRichVariants(QStringLiteral("CreateBOM_Overall"));
    }
    if (
        normalizedLower == QStringLiteral("__commandtab_toggle_grid__")
        || normalizedLower == QStringLiteral("draft_togglegrid")
        || normalizedLower == QStringLiteral("sketcher_grid")
    ) {
        appendRichVariants(QStringLiteral("Sketcher_GridToggle_Deactivated.svg"));
        appendRichVariants(QStringLiteral("Draft_ToggleGrid"));
        appendRichVariants(QStringLiteral("Sketcher_Grid"));
        appendRichVariants(QStringLiteral("view-grid"));
    }
    if (normalizedLower == QStringLiteral("edit tools")) {
        appendRichVariants(QStringLiteral("Sketcher_EditSketch"));
        appendRichVariants(QStringLiteral("modern_cmd_sketch_edit.svg"));
    }

    QStringList orderedCandidates;
    QSet<QString> seen;
    for (const auto& candidate : candidates) {
        if (candidate.isEmpty() || seen.contains(candidate)) {
            continue;
        }
        orderedCandidates.push_back(candidate);
        seen.insert(candidate);
    }
    return orderedCandidates;
}

QString directCommandIconSource(const QString& commandId)
{
    for (const auto& candidate : commandIconCandidates(commandId)) {
        const QString resolvedSource = directlyLoadableIconSource(candidate);
        if (!resolvedSource.isEmpty()) {
            return resolvedSource;
        }
    }
    return QString();
}

bool loadCommandOverrides(const QString& addonRootPath, CommandTabCommandOverrides* overrides)
{
    if (overrides == nullptr) {
        return false;
    }

    const auto cachedIt = g_commandOverridesCache.constFind(addonRootPath);
    if (cachedIt != g_commandOverridesCache.constEnd()) {
        *overrides = cachedIt.value();
        return true;
    }

    CommandTabCommandOverrides loadedOverrides;
    const QString overridesPath = QDir(addonRootPath).absoluteFilePath(
        QStringLiteral("freecad_commandtab/native/command_overrides.json")
    );
    QFile file(overridesPath);
    if (!file.open(QIODevice::ReadOnly)) {
        g_commandOverridesCache.insert(addonRootPath, loadedOverrides);
        *overrides = loadedOverrides;
        return false;
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        g_commandOverridesCache.insert(addonRootPath, loadedOverrides);
        *overrides = loadedOverrides;
        return false;
    }

    const auto root = document.object();
    const auto specialInfoObject = root.value(QStringLiteral("specialCommandInfoUpdates")).toObject();
    for (auto it = specialInfoObject.begin(); it != specialInfoObject.end(); ++it) {
        if (it.value().isObject()) {
            loadedOverrides.specialCommandInfoUpdates.insert(it.key(), it.value().toObject());
        }
    }

    const auto specialPixmapsObject = root.value(QStringLiteral("specialPixmaps")).toObject();
    for (auto it = specialPixmapsObject.begin(); it != specialPixmapsObject.end(); ++it) {
        loadedOverrides.specialPixmaps.insert(it.key(), it.value().toString());
    }

    const auto customPixmapsObject = root.value(QStringLiteral("customCommandPixmapFiles")).toObject();
    for (auto it = customPixmapsObject.begin(); it != customPixmapsObject.end(); ++it) {
        loadedOverrides.customCommandPixmapFiles.insert(it.key(), it.value().toString());
    }

    g_commandOverridesCache.insert(addonRootPath, loadedOverrides);
    *overrides = loadedOverrides;
    return true;
}

QString resolveIconPath(const QString& addonRootPath, const QString& iconValue)
{
    const QString normalized = iconValue.trimmed();
    if (normalized.isEmpty()) {
        return QString();
    }

    const QString directlyLoadable = directlyLoadableIconSource(normalized);
    if (!directlyLoadable.isEmpty()) {
        return directlyLoadable;
    }

    const QFileInfo originalInfo(normalized);
    if (originalInfo.isAbsolute() && originalInfo.exists()) {
        return originalInfo.absoluteFilePath();
    }

    const QString addonRelativePath = QDir(addonRootPath).absoluteFilePath(normalized);
    if (QFileInfo::exists(addonRelativePath)) {
        return addonRelativePath;
    }

    QString relativeIconName = normalized;
    if (relativeIconName.startsWith(QStringLiteral("icons:"))) {
        relativeIconName = relativeIconName.mid(QStringLiteral("icons:").size());
    }
    relativeIconName.replace(QLatin1Char('\\'), QLatin1Char('/'));
    while (relativeIconName.startsWith(QLatin1Char('/'))) {
        relativeIconName.remove(0, 1);
    }

    const QString iconFileName = QFileInfo(relativeIconName).fileName();
    if (relativeIconName.isEmpty() && iconFileName.isEmpty()) {
        return QString();
    }

    QStringList fileCandidates;
    auto appendFileCandidate = [&](QString candidate) {
        candidate = QDir::cleanPath(candidate.trimmed());
        if (candidate.isEmpty() || candidate == QStringLiteral(".")) {
            return;
        }
        if (!fileCandidates.contains(candidate)) {
            fileCandidates.push_back(candidate);
        }
    };

    QStringList rawCandidates;
    if (!relativeIconName.isEmpty()) {
        rawCandidates.push_back(relativeIconName);
    }
    if (!iconFileName.isEmpty() && iconFileName != relativeIconName) {
        rawCandidates.push_back(iconFileName);
    }

    for (const auto& rawCandidate : rawCandidates) {
        const QFileInfo candidateInfo(rawCandidate);
        if (!candidateInfo.suffix().isEmpty()) {
            appendFileCandidate(rawCandidate);
            continue;
        }
        for (const auto& extension : bundledIconExtensions()) {
            appendFileCandidate(rawCandidate + extension);
        }
    }

    QStringList searchDirectories;
    QSet<QString> seenDirectories;
    auto appendSearchDirectory = [&](const QString& directoryPath) {
        const QString cleanedPath = QDir::cleanPath(directoryPath.trimmed());
        if (cleanedPath.isEmpty() || seenDirectories.contains(cleanedPath)) {
            return;
        }
        seenDirectories.insert(cleanedPath);
        searchDirectories.push_back(cleanedPath);
    };

    for (const auto& searchPath : QDir::searchPaths(QStringLiteral("icons"))) {
        appendSearchDirectory(searchPath);
    }

    const QString activeThemeName = QIcon::themeName().trimmed();
    for (const auto& themeRoot : QIcon::themeSearchPaths()) {
        appendSearchDirectory(themeRoot);
        appendSearchDirectory(QDir(themeRoot).absoluteFilePath(QStringLiteral("scalable")));
        if (!activeThemeName.isEmpty()) {
            appendSearchDirectory(QDir(themeRoot).absoluteFilePath(activeThemeName));
            appendSearchDirectory(
                QDir(themeRoot).absoluteFilePath(activeThemeName + QStringLiteral("/scalable"))
            );
        }
    }

    auto appendModuleIconDirectories = [&](const QString& modRootPath) {
        const QDir modRoot(modRootPath);
        if (!modRoot.exists()) {
            return;
        }

        const auto moduleNames = modRoot.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const auto& moduleName : moduleNames) {
            appendSearchDirectory(
                modRoot.absoluteFilePath(moduleName + QStringLiteral("/Resources/icons"))
            );
        }
    };

    const QDir applicationBinDir(QCoreApplication::applicationDirPath());
    const QString applicationResourcesPath = QDir::cleanPath(
        applicationBinDir.absoluteFilePath(QStringLiteral(".."))
    );
    appendModuleIconDirectories(QDir(applicationResourcesPath).absoluteFilePath(QStringLiteral("share/Mod")));
    appendModuleIconDirectories(QDir(applicationResourcesPath).absoluteFilePath(QStringLiteral("Mod")));

    // Keep addon-bundled icons as a final fallback only.
    appendSearchDirectory(
        QDir(addonRootPath).absoluteFilePath(
            QStringLiteral("Export/IconThemes/FreeCAD-CommandTab-Modern/scalable")
        )
    );
    appendSearchDirectory(QDir(addonRootPath).absoluteFilePath(QStringLiteral("Resources/icons")));

    for (const auto& directory : searchDirectories) {
        const QDir searchDir(directory);
        for (const auto& fileCandidate : fileCandidates) {
            const QString fullPath = searchDir.absoluteFilePath(fileCandidate);
            if (QFileInfo::exists(fullPath)) {
                return fullPath;
            }
        }
    }

    return QString();
}

QString preferredPixmapFile(
    const QString& commandId,
    const CommandTabCommandOverrides& overrides
)
{
    for (const auto& candidate : commandIconCandidates(commandId)) {
        const auto specialIt = overrides.specialPixmaps.constFind(candidate);
        if (specialIt != overrides.specialPixmaps.constEnd()) {
            const QString value = specialIt.value().trimmed();
            if (!value.isEmpty()) {
                return value;
            }
        }
    }

    for (const auto& candidate : commandIconCandidates(commandId)) {
        const auto customIt = overrides.customCommandPixmapFiles.constFind(candidate);
        if (customIt != overrides.customCommandPixmapFiles.constEnd()) {
            const QString value = customIt.value().trimmed();
            if (!value.isEmpty()) {
                return value;
            }
        }
    }
    return QString();
}

void mergeStructureCommandDefinition(
    QHash<QString, QJsonObject>* commandDefinitions,
    const QString& commandId,
    const QJsonObject& source
)
{
    if (commandDefinitions == nullptr || commandId.isEmpty()) {
        return;
    }

    QJsonObject definition = commandDefinitions->value(commandId);
    if (definition.value(QStringLiteral("text")).toString().isEmpty()) {
        const QString text = source.value(QStringLiteral("text")).toString();
        if (!text.isEmpty()) {
            definition.insert(QStringLiteral("text"), text);
        }
    }
    if (definition.value(QStringLiteral("icon")).toString().isEmpty()) {
        const QString icon = source.value(QStringLiteral("icon")).toString();
        if (!icon.isEmpty()) {
            definition.insert(QStringLiteral("icon"), icon);
        }
    }
    if (definition.value(QStringLiteral("size")).toString().isEmpty()) {
        const QString size = source.value(QStringLiteral("size")).toString().trimmed().toLower();
        if (
            size == QStringLiteral("small")
            || size == QStringLiteral("medium")
            || size == QStringLiteral("large")
        ) {
            definition.insert(QStringLiteral("size"), size);
        }
    }
    commandDefinitions->insert(commandId, definition);
}

void applyOverrideMetadata(
    QJsonObject* commandsObject,
    const QString& commandId,
    const CommandTabCommandOverrides& overrides,
    const QString& addonRootPath,
    bool useThemeOverrides
)
{
    if (commandsObject == nullptr || commandId.isEmpty()) {
        return;
    }

    QJsonObject commandObject = commandsObject->value(commandId).toObject();

    const auto specialInfoIt = overrides.specialCommandInfoUpdates.constFind(commandId);
    if (specialInfoIt != overrides.specialCommandInfoUpdates.constEnd()) {
        const QString menuText = specialInfoIt->value(QStringLiteral("menuText")).toString().trimmed();
        if (!menuText.isEmpty() && commandObject.value(QStringLiteral("text")).toString().isEmpty()) {
            commandObject.insert(QStringLiteral("text"), menuText);
        }
    }

    const QString currentIconPath = commandObject.value(QStringLiteral("iconPath")).toString().trimmed();
    QString normalizedCurrentIconPath = currentIconPath;
    normalizedCurrentIconPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
    const bool currentLooksTransientQtCache = normalizedCurrentIconPath.contains(QStringLiteral("/qt-action-icons/"));
    const bool currentIconLikelyTooSmall = isLikelyLowResolutionIconFile(currentIconPath, 20);

    QString overridePixmap;
    if (useThemeOverrides) {
        overridePixmap = preferredPixmapFile(commandId, overrides);
    } else {
        // Always honor special pixmap overrides for commands that are missing a stable icon.
        for (const auto& candidate : commandIconCandidates(commandId)) {
            const auto specialIt = overrides.specialPixmaps.constFind(candidate);
            if (specialIt != overrides.specialPixmaps.constEnd()) {
                const QString specialValue = specialIt.value().trimmed();
                if (!specialValue.isEmpty()) {
                    overridePixmap = specialValue;
                    break;
                }
            }
        }
    }

    if (
        !overridePixmap.isEmpty()
        && (currentIconPath.isEmpty() || currentLooksTransientQtCache || currentIconLikelyTooSmall)
    ) {
        QString resolvedIconPath = directlyLoadableIconSource(overridePixmap);
        if (resolvedIconPath.isEmpty()) {
            resolvedIconPath = resolveIconPath(addonRootPath, overridePixmap);
        }
        if (resolvedIconPath.isEmpty()) {
            const QIcon icon = loadIconFromSource(overridePixmap);
            if (!icon.isNull()) {
                resolvedIconPath = overridePixmap;
            }
        }
        if (!resolvedIconPath.isEmpty()) {
            commandObject.insert(QStringLiteral("iconPath"), resolvedIconPath);
        }
    }

    commandsObject->insert(commandId, commandObject);
}

void applyStructureFallbackMetadata(
    QJsonObject* commandsObject,
    const QString& commandId,
    const QHash<QString, QJsonObject>& commandDefinitions,
    const QString& addonRootPath,
    const QString& iconDirectoryPath
)
{
    if (commandsObject == nullptr || commandId.isEmpty()) {
        return;
    }

    const auto definitionIt = commandDefinitions.constFind(commandId);
    if (definitionIt == commandDefinitions.constEnd()) {
        return;
    }

    QJsonObject commandObject = commandsObject->value(commandId).toObject();
    const QString currentText = commandObject.value(QStringLiteral("text")).toString();
    if (currentText.isEmpty() || looksLikeTechnicalCommandText(currentText, commandId)) {
        const QString text = definitionIt->value(QStringLiteral("text")).toString().trimmed();
        if (!text.isEmpty()) {
            commandObject.insert(QStringLiteral("text"), text);
        }
    }
    const QString currentIconPath = commandObject.value(QStringLiteral("iconPath")).toString().trimmed();
    QString normalizedCurrentIconPath = currentIconPath;
    normalizedCurrentIconPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
    const bool currentLooksTransientQtCache = normalizedCurrentIconPath.contains(QStringLiteral("/qt-action-icons/"));
    const QString definitionSize = definitionIt->value(QStringLiteral("size")).toString().trimmed().toLower();
    int minimumIconEdge = 0;
    if (definitionSize == QStringLiteral("large")) {
        minimumIconEdge = 28;
    } else if (definitionSize == QStringLiteral("medium")) {
        minimumIconEdge = 20;
    }
    const bool currentIconTooSmall =
        minimumIconEdge > 0 && isLikelyLowResolutionIconFile(currentIconPath, minimumIconEdge);
    if (currentIconPath.isEmpty() || currentLooksTransientQtCache || currentIconTooSmall) {
        QStringList iconCandidates;
        const QString iconValue = definitionIt->value(QStringLiteral("icon")).toString().trimmed();
        if (!iconValue.isEmpty()) {
            iconCandidates.push_back(iconValue);
        }
        for (const auto& candidate : commandIconCandidates(commandId)) {
            if (!candidate.isEmpty() && !iconCandidates.contains(candidate)) {
                iconCandidates.push_back(candidate);
            }
        }

        QString resolvedIconPath;
        for (const auto& candidate : iconCandidates) {
            const QString stableIconSource = resolveIconPath(addonRootPath, candidate);
            if (isDirectFileOrResourceIconSource(stableIconSource)) {
                resolvedIconPath = stableIconSource;
                const QIcon stableIcon = loadIconFromSource(stableIconSource);
                const QString exportedPath = exportActionIconPath(commandId, stableIcon, iconDirectoryPath);
                if (!exportedPath.isEmpty()) {
                    resolvedIconPath = exportedPath;
                }
            } else {
                const QIcon icon = loadIconFromSource(candidate);
                if (!icon.isNull()) {
                    resolvedIconPath = exportActionIconPath(commandId, icon, iconDirectoryPath);
                }
                if (resolvedIconPath.isEmpty()) {
                    resolvedIconPath = stableIconSource;
                }
            }
            if (!resolvedIconPath.isEmpty()) {
                break;
            }
        }

        if (!resolvedIconPath.isEmpty()) {
            commandObject.insert(QStringLiteral("iconPath"), resolvedIconPath);
        }
    }

    commandsObject->insert(commandId, commandObject);
}

bool parseMetadataCacheFromPath(const QString& path, CommandTabMetadataCache* cache)
{
    if (cache == nullptr) {
        setLastError(QStringLiteral("Metadata cache output is null"));
        return false;
    }

    cache->commands.clear();
    cache->workbenchTitles.clear();
    cache->workbenchIcons.clear();
    cache->panelTitles.clear();
    if (path.isEmpty()) {
        return true;
    }

    QJsonObject root;
    if (!loadJsonObjectFromFile(path, &root, QStringLiteral("native commandtab metadata cache"))) {
        return false;
    }

    const auto commandsObject = root.value(QStringLiteral("commands")).toObject();
    for (auto it = commandsObject.begin(); it != commandsObject.end(); ++it) {
        if (!it.value().isObject()) {
            continue;
        }
        const auto commandObject = it.value().toObject();
        CommandTabMetadataEntry entry;
        entry.text = commandObject.value(QStringLiteral("text")).toString();
        entry.iconPath = commandObject.value(QStringLiteral("iconPath")).toString();
        cache->commands.insert(it.key(), entry);
    }

    const auto workbenchTitlesObject = root.value(QStringLiteral("workbenchTitles")).toObject();
    for (auto it = workbenchTitlesObject.begin(); it != workbenchTitlesObject.end(); ++it) {
        cache->workbenchTitles.insert(it.key(), it.value().toString(it.key()));
    }

    const auto workbenchIconsObject = root.value(QStringLiteral("workbenchIcons")).toObject();
    for (auto it = workbenchIconsObject.begin(); it != workbenchIconsObject.end(); ++it) {
        cache->workbenchIcons.insert(it.key(), it.value().toString());
    }

    const auto panelTitlesObject = root.value(QStringLiteral("panelTitles")).toObject();
    for (auto it = panelTitlesObject.begin(); it != panelTitlesObject.end(); ++it) {
        cache->panelTitles.insert(it.key(), it.value().toString(it.key()));
    }
    return true;
}

QString normalizeActionCandidate(QString value)
{
    value = value.trimmed();
    if (value.isEmpty()) {
        return QString();
    }
    const qsizetype tabIndex = value.indexOf(QLatin1Char('\t'));
    if (tabIndex >= 0) {
        value = value.left(tabIndex).trimmed();
    }
    return value;
}

bool looksLikeCommandId(const QString& value)
{
    if (value.isEmpty()) {
        return false;
    }
    const QString normalized = value.trimmed();
    const qsizetype commaIndex = normalized.indexOf(QLatin1Char(','));
    if (commaIndex > 0) {
        const QString left = normalized.left(commaIndex).trimmed();
        const QString right = normalized.mid(commaIndex + 1).trimmed();
        const bool rightIsIndex = !right.isEmpty()
            && std::all_of(right.begin(), right.end(), [](QChar character) {
                return character.isDigit();
            });
        const bool leftHasWhitespace = std::any_of(left.begin(), left.end(), [](QChar character) {
            return character.isSpace();
        });
        if (!left.isEmpty() && !leftHasWhitespace && rightIsIndex) {
            return true;
        }
    }
    for (const auto character : value) {
        if (character.isSpace()) {
            return false;
        }
    }
    return true;
}

bool looksLikeTechnicalCommandText(const QString& value, const QString& commandId)
{
    QString normalized = normalizeActionCandidate(value);
    normalized.replace(QStringLiteral("&"), QString());
    normalized = normalized.trimmed();
    if (normalized.isEmpty()) {
        return false;
    }

    for (const auto& candidate : commandIconCandidates(commandId)) {
        if (!candidate.isEmpty() && normalized.compare(candidate, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    const bool hasTokenSeparators =
        normalized.contains(QLatin1Char('_')) || normalized.contains(QLatin1Char(','));
    if (hasTokenSeparators && looksLikeCommandId(normalized)) {
        return true;
    }

    return false;
}

bool isLikelyLowResolutionIconFile(const QString& iconPath, int minEdgePixels)
{
    if (minEdgePixels <= 0) {
        return false;
    }
    const QString normalizedPath = iconPath.trimmed();
    if (normalizedPath.isEmpty() || normalizedPath.startsWith(QStringLiteral(":/"))) {
        return false;
    }
    const QFileInfo iconInfo(normalizedPath);
    if (!iconInfo.exists() || !iconInfo.isFile()) {
        return false;
    }

    QPixmap pixmap(normalizedPath);
    if (pixmap.isNull()) {
        return false;
    }
    const QSize size = pixmap.size();
    return size.width() < minEdgePixels || size.height() < minEdgePixels;
}

QString actionCommandId(QAction* action)
{
    if (action == nullptr) {
        return QString();
    }

    const QList<QString> candidates = {
        normalizeActionCandidate(action->objectName()),
        normalizeActionCandidate(action->property("Command").toString()),
        normalizeActionCandidate(action->property("command").toString()),
        normalizeActionCandidate(action->property("actionName").toString()),
        normalizeActionCandidate(action->data().toString()),
    };

    for (const auto& candidate : candidates) {
        if (looksLikeCommandId(candidate)) {
            return candidate;
        }
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const auto associatedObjects = action->associatedObjects();
    for (auto* object : associatedObjects) {
        auto* widget = qobject_cast<QWidget*>(object);
        if (widget == nullptr) {
            continue;
        }
        const QList<QString> widgetCandidates = {
            normalizeActionCandidate(widget->objectName()),
            normalizeActionCandidate(widget->property("Command").toString()),
            normalizeActionCandidate(widget->property("command").toString()),
        };
        for (const auto& candidate : widgetCandidates) {
            if (looksLikeCommandId(candidate)) {
                return candidate;
            }
        }
    }
#else
    const auto widgets = action->associatedWidgets();
    for (auto* widget : widgets) {
        if (widget == nullptr) {
            continue;
        }
        const QList<QString> widgetCandidates = {
            normalizeActionCandidate(widget->objectName()),
            normalizeActionCandidate(widget->property("Command").toString()),
            normalizeActionCandidate(widget->property("command").toString()),
        };
        for (const auto& candidate : widgetCandidates) {
            if (looksLikeCommandId(candidate)) {
                return candidate;
            }
        }
    }
#endif
    return QString();
}

QString cleanedActionText(QAction* action)
{
    if (action == nullptr) {
        return QString();
    }

    QString text = action->iconText().trimmed();
    if (text.isEmpty()) {
        text = action->text().trimmed();
    }
    if (text.isEmpty()) {
        text = action->toolTip().trimmed();
    }
    const qsizetype tabIndex = text.indexOf(QLatin1Char('\t'));
    if (tabIndex >= 0) {
        text = text.left(tabIndex);
    }
    text.replace(QStringLiteral("&"), QString());
    return text.trimmed();
}

QString cleanedActionDisplayText(QAction* action, const QString& commandId)
{
    if (action == nullptr) {
        return QString();
    }

    const QStringList candidates = {
        action->text(),
        action->iconText(),
        action->toolTip(),
        action->statusTip(),
    };

    QString firstCleaned;
    for (QString candidate : candidates) {
        const qsizetype tabIndex = candidate.indexOf(QLatin1Char('\t'));
        if (tabIndex >= 0) {
            candidate = candidate.left(tabIndex);
        }
        candidate.replace(QStringLiteral("&"), QString());
        candidate = candidate.trimmed();
        if (candidate.isEmpty()) {
            continue;
        }
        if (firstCleaned.isEmpty()) {
            firstCleaned = candidate;
        }
        if (!looksLikeTechnicalCommandText(candidate, commandId)) {
            return candidate;
        }
    }

    return firstCleaned;
}

QString cleanedToolbarTitle(QToolBar* toolbar)
{
    if (toolbar == nullptr) {
        return QString();
    }

    QString title;
    if (toolbar->toggleViewAction() != nullptr) {
        title = cleanedActionText(toolbar->toggleViewAction());
    }
    if (title.isEmpty()) {
        title = toolbar->windowTitle().trimmed();
    }
    if (title.isEmpty()) {
        title = toolbar->objectName().trimmed();
    }
    title.replace(QStringLiteral("&"), QString());
    return title.trimmed();
}

QString safeIconFileStem(QString value)
{
    if (value.isEmpty()) {
        value = QStringLiteral("action");
    }
    for (auto& character : value) {
        if (!character.isLetterOrNumber() && character != QLatin1Char('_') && character != QLatin1Char('-')) {
            character = QLatin1Char('_');
        }
    }
    return value;
}

QString exportActionIconPath(const QString& commandId, const QIcon& icon, const QString& iconDirectoryPath)
{
    if (commandId.isEmpty() || icon.isNull() || iconDirectoryPath.isEmpty()) {
        return QString();
    }

    QDir iconDirectory(iconDirectoryPath);
    if (!iconDirectory.exists() && !iconDirectory.mkpath(QStringLiteral("."))) {
        return QString();
    }

    QPixmap pixmap = icon.pixmap(64, 64);
    if (pixmap.isNull()) {
        pixmap = icon.pixmap(48, 48);
    }
    if (pixmap.isNull()) {
        return QString();
    }

    const QByteArray digest = QCryptographicHash::hash(
        commandId.toUtf8() + '|' + QByteArray::number(static_cast<qulonglong>(pixmap.cacheKey())),
        QCryptographicHash::Sha1
    ).toHex();
    const QString filePath = iconDirectory.absoluteFilePath(
        QStringLiteral("%1_%2.png").arg(safeIconFileStem(commandId), QString::fromLatin1(digest.left(12)))
    );
    if (QFileInfo::exists(filePath)) {
        return filePath;
    }
    if (!pixmap.save(filePath, "PNG")) {
        return QString();
    }
    return filePath;
}

QString exportActionIconSource(const QString& commandId, const QIcon& icon, const QString& iconDirectoryPath)
{
    const QString exportedSource = exportActionIconPath(commandId, icon, iconDirectoryPath);
    if (!exportedSource.isEmpty()) {
        return exportedSource;
    }
    return directCommandIconSource(commandId);
}

void mergeActionMetadata(
    QJsonObject* commandsObject,
    QAction* action,
    const QString& iconDirectoryPath,
    const QSet<QString>* allowedCommands = nullptr
)
{
    if (commandsObject == nullptr || action == nullptr) {
        return;
    }

    const QString commandId = actionCommandId(action);
    if (commandId.isEmpty() || isSeparatorCommand(commandId)) {
        return;
    }
    if (allowedCommands != nullptr && !allowedCommands->contains(commandId)) {
        return;
    }

    QJsonObject commandObject = commandsObject->value(commandId).toObject();
    const QString text = cleanedActionDisplayText(action, commandId);
    if (!text.isEmpty() && !looksLikeTechnicalCommandText(text, commandId)) {
        commandObject.insert(QStringLiteral("text"), text);
    }
    if (commandObject.value(QStringLiteral("toolTip")).toString().isEmpty()) {
        const QString toolTip = action->toolTip().trimmed();
        if (!toolTip.isEmpty()) {
            commandObject.insert(QStringLiteral("toolTip"), toolTip);
        }
    }
    if (commandObject.value(QStringLiteral("iconPath")).toString().isEmpty()) {
        const QString iconPath = exportActionIconSource(commandId, action->icon(), iconDirectoryPath);
        if (!iconPath.isEmpty()) {
            commandObject.insert(QStringLiteral("iconPath"), iconPath);
        }
    }
    commandsObject->insert(commandId, commandObject);
}

QVector<QAction*> collectUniqueWindowActions(QMainWindow* mainWindow)
{
    QVector<QAction*> actions;
    if (mainWindow == nullptr) {
        return actions;
    }

    QSet<QAction*> seen;
    const auto toolbars = mainWindow->findChildren<QToolBar*>();
    for (auto* toolbar : toolbars) {
        if (toolbar == nullptr) {
            continue;
        }
        for (auto* action : toolbar->actions()) {
            if (action == nullptr || seen.contains(action)) {
                continue;
            }
            seen.insert(action);
            actions.push_back(action);
        }
    }

    const auto childActions = mainWindow->findChildren<QAction*>();
    for (auto* action : childActions) {
        if (action == nullptr || seen.contains(action)) {
            continue;
        }
        seen.insert(action);
        actions.push_back(action);
    }
    return actions;
}

bool exportQtActionCache(QMainWindow* mainWindow, const QString& outputPath, const QString& iconDirectoryPath)
{
    if (mainWindow == nullptr) {
        setLastError(QStringLiteral("Missing main window for Qt action cache export"));
        return false;
    }
    if (outputPath.isEmpty()) {
        setLastError(QStringLiteral("Missing output path for Qt action cache export"));
        return false;
    }

    QJsonObject commandsObject;
    const auto actions = collectUniqueWindowActions(mainWindow);
    for (auto* action : actions) {
        mergeActionMetadata(&commandsObject, action, iconDirectoryPath);
    }

    QJsonObject root;
    root.insert(QStringLiteral("commands"), commandsObject);

    QFile outputFile(outputPath);
    const QFileInfo outputInfo(outputFile);
    QDir().mkpath(outputInfo.absolutePath());
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setLastError(QStringLiteral("Unable to write Qt action cache: %1").arg(outputPath));
        return false;
    }
    outputFile.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    outputFile.close();
    return true;
}

QSet<QString> parseWorkbenchIdFilter(const QByteArray& workbenchIdsJson)
{
    QSet<QString> workbenchIds;
    if (workbenchIdsJson.trimmed().isEmpty()) {
        return workbenchIds;
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(workbenchIdsJson, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
        return workbenchIds;
    }

    const auto array = document.array();
    for (const auto& value : array) {
        const QString workbenchId = value.toString().trimmed();
        if (!workbenchId.isEmpty()) {
            workbenchIds.insert(workbenchId);
        }
    }
    return workbenchIds;
}

void collectStructureMetadataRequirements(
    const QJsonObject& structureRoot,
    const QSet<QString>& requestedWorkbenchIds,
    bool includeQuickAccess,
    QSet<QString>* commandIds,
    QJsonObject* workbenchTitlesObject,
    QJsonObject* panelTitlesObject,
    QJsonArray* builtWorkbenches,
    QHash<QString, QJsonObject>* commandDefinitions
)
{
    if (
        commandIds == nullptr
        || workbenchTitlesObject == nullptr
        || panelTitlesObject == nullptr
        || builtWorkbenches == nullptr
    ) {
        return;
    }

    if (includeQuickAccess) {
        const auto quickAccessArray = structureRoot.value(QStringLiteral("quickAccessCommands")).toArray();
        for (const auto& value : quickAccessArray) {
            const QString commandId = value.toString().trimmed();
            if (!commandId.isEmpty() && !isSeparatorCommand(commandId)) {
                commandIds->insert(commandId);
                mergeStructureCommandDefinition(commandDefinitions, commandId, QJsonObject());
                if (commandId.endsWith(QStringLiteral("_ddb"))) {
                    collectDropdownMetadataRequirements(structureRoot, commandId, commandIds, commandDefinitions);
                }
            }
        }
    }

    const auto workbenchesObject = structureRoot.value(QStringLiteral("workbenches")).toObject();
    for (auto it = workbenchesObject.begin(); it != workbenchesObject.end(); ++it) {
        if (!it.value().isObject()) {
            continue;
        }

        const QString workbenchId = it.key();
        if (!requestedWorkbenchIds.isEmpty() && !requestedWorkbenchIds.contains(workbenchId)) {
            continue;
        }

        const auto workbenchObject = it.value().toObject();
        if (workbenchHasEnabledPanels(structureRoot, workbenchId, workbenchObject) == false) {
            continue;
        }

        const QString workbenchTitle = workbenchObject.value(QStringLiteral("title")).toString(workbenchId);
        workbenchTitlesObject->insert(workbenchId, workbenchTitle);
        builtWorkbenches->push_back(workbenchId);

        const auto toolbarsObject = workbenchObject.value(QStringLiteral("toolbars")).toObject();
        for (const auto& panelId : orderedToolbarIds(toolbarsObject)) {
            const auto panelObject = toolbarsObject.value(panelId).toObject();
            if (isIgnoredToolbar(structureRoot, workbenchId, panelId, panelObject)) {
                continue;
            }
            if (panelObject.value(QStringLiteral("Enabled")).toBool(true) == false) {
                continue;
            }

            panelTitlesObject->insert(
                panelId,
                panelObject.value(QStringLiteral("title")).toString(panelId)
            );

            for (const auto& commandId : orderedCommandIds(panelObject)) {
                if (!commandId.isEmpty() && !isSeparatorCommand(commandId)) {
                    commandIds->insert(commandId);
                    mergeStructureCommandDefinition(
                        commandDefinitions,
                        commandId,
                        panelObject.value(QStringLiteral("commands")).toObject().value(commandId).toObject()
                    );
                    if (commandId.endsWith(QStringLiteral("_ddb"))) {
                        collectDropdownMetadataRequirements(structureRoot, commandId, commandIds, commandDefinitions);
                    }
                }
            }
        }

        const auto customPanelsObject = structureRoot.value(QStringLiteral("customToolbars")).toObject().value(workbenchId).toObject();
        for (auto customIt = customPanelsObject.begin(); customIt != customPanelsObject.end(); ++customIt) {
            if (!customIt.value().isObject()) {
                continue;
            }
            const QString panelId = customIt.key();
            if (isIgnoredToolbar(structureRoot, workbenchId, panelId, customIt.value().toObject())) {
                continue;
            }
            panelTitlesObject->insert(panelId, panelTitleFromId(panelId, QStringLiteral("_custom")));
            const auto commandsObject = customIt.value().toObject().value(QStringLiteral("commands")).toObject();
            for (auto commandIt = commandsObject.begin(); commandIt != commandsObject.end(); ++commandIt) {
                const QString commandId = commandIt.key().trimmed();
                if (commandId.isEmpty() || isSeparatorCommand(commandId)) {
                    continue;
                }
                commandIds->insert(commandId);
                mergeStructureCommandDefinition(commandDefinitions, commandId, QJsonObject());
                if (commandId.endsWith(QStringLiteral("_ddb"))) {
                    collectDropdownMetadataRequirements(structureRoot, commandId, commandIds, commandDefinitions);
                }
            }
        }

        const auto newPanelsRoot = structureRoot.value(QStringLiteral("newPanels")).toObject();
        for (const QString& source : {workbenchId, QStringLiteral("Global")}) {
            const auto newPanelsObject = newPanelsRoot.value(source).toObject();
            for (auto panelIt = newPanelsObject.begin(); panelIt != newPanelsObject.end(); ++panelIt) {
                const QString panelId = panelIt.key();
                if (isIgnoredToolbar(structureRoot, source, panelId, QJsonObject())) {
                    continue;
                }
                if (!panelIt.value().isArray()) {
                    continue;
                }
                panelTitlesObject->insert(panelId, panelTitleFromId(panelId, QStringLiteral("_newPanel")));
                const auto commandsArray = panelIt.value().toArray();
                for (const auto& commandValue : commandsArray) {
                    if (!commandValue.isArray()) {
                        continue;
                    }
                    const auto commandArray = commandValue.toArray();
                    if (commandArray.isEmpty()) {
                        continue;
                    }
                    const QString commandId = commandArray.at(0).toString().trimmed();
                    if (commandId.isEmpty() || isSeparatorCommand(commandId)) {
                        continue;
                    }
                    commandIds->insert(commandId);
                    mergeStructureCommandDefinition(commandDefinitions, commandId, QJsonObject());
                    if (commandId.endsWith(QStringLiteral("_ddb"))) {
                        collectDropdownMetadataRequirements(structureRoot, commandId, commandIds, commandDefinitions);
                    }
                }
            }
        }
    }
}

bool exportStructureMetadataCache(
    QMainWindow* mainWindow,
    const QString& structurePath,
    const QString& outputPath,
    const QString& iconDirectoryPath,
    const QByteArray& workbenchIdsJson,
    bool includeQuickAccess,
    bool useThemeOverrides
)
{
    if (mainWindow == nullptr) {
        setLastError(QStringLiteral("Missing main window for structure metadata export"));
        return false;
    }
    if (structurePath.isEmpty()) {
        setLastError(QStringLiteral("Missing structure path for structure metadata export"));
        return false;
    }
    if (outputPath.isEmpty()) {
        setLastError(QStringLiteral("Missing output path for structure metadata export"));
        return false;
    }

    QJsonObject structureRoot;
    if (!loadJsonObjectFromFile(structurePath, &structureRoot, QStringLiteral("native commandtab structure"))) {
        return false;
    }

    const QSet<QString> requestedWorkbenchIds = parseWorkbenchIdFilter(workbenchIdsJson);
    QSet<QString> commandIds;
    QJsonObject workbenchTitlesObject;
    QJsonObject panelTitlesObject;
    QJsonArray builtWorkbenches;
    QHash<QString, QJsonObject> commandDefinitions;
    collectStructureMetadataRequirements(
        structureRoot,
        requestedWorkbenchIds,
        includeQuickAccess,
        &commandIds,
        &workbenchTitlesObject,
        &panelTitlesObject,
        &builtWorkbenches,
        &commandDefinitions
    );

    QJsonObject commandsObject;
    QJsonObject workbenchIconsObject;
    const QString addonRootPath = addonRootFromStructurePath(structurePath);
    CommandTabCommandOverrides commandOverrides;
    loadCommandOverrides(addonRootPath, &commandOverrides);
    const auto toolbars = mainWindow->findChildren<QToolBar*>();
    for (auto* toolbar : toolbars) {
        if (toolbar == nullptr) {
            continue;
        }
        const QString panelId = toolbar->objectName().trimmed();
        if (!panelId.isEmpty() && panelTitlesObject.contains(panelId)) {
            const QString panelTitle = cleanedToolbarTitle(toolbar);
            if (!panelTitle.isEmpty()) {
                panelTitlesObject.insert(panelId, panelTitle);
            }
        }
        const auto actions = toolbar->actions();
        for (auto* action : actions) {
            mergeActionMetadata(&commandsObject, action, iconDirectoryPath, &commandIds);
        }
    }

    const auto actions = collectUniqueWindowActions(mainWindow);
    for (auto* action : actions) {
        mergeActionMetadata(&commandsObject, action, iconDirectoryPath, &commandIds);
    }

    for (const auto& commandId : commandIds) {
        applyStructureFallbackMetadata(
            &commandsObject,
            commandId,
            commandDefinitions,
            addonRootPath,
            iconDirectoryPath
        );
        applyOverrideMetadata(&commandsObject, commandId, commandOverrides, addonRootPath, useThemeOverrides);
    }

    for (const auto& workbenchValue : builtWorkbenches) {
        const QString workbenchId = workbenchValue.toString().trimmed();
        if (workbenchId.isEmpty()) {
            continue;
        }
        const QString workbenchTitle = workbenchTitlesObject.value(workbenchId).toString(workbenchId);
        const QString iconSource = resolveWorkbenchIconSource(addonRootPath, workbenchId, workbenchTitle);
        if (!iconSource.isEmpty()) {
            const QIcon workbenchIcon = loadIconFromSource(iconSource);
            const QString exportedIconSource = exportActionIconPath(
                QStringLiteral("workbench_%1").arg(workbenchId),
                workbenchIcon,
                iconDirectoryPath
            );
            workbenchIconsObject.insert(
                workbenchId,
                exportedIconSource.isEmpty() ? iconSource : exportedIconSource
            );
        }
    }

    QJsonObject root;
    root.insert(QStringLiteral("commands"), commandsObject);
    root.insert(QStringLiteral("workbenchTitles"), workbenchTitlesObject);
    root.insert(QStringLiteral("workbenchIcons"), workbenchIconsObject);
    root.insert(QStringLiteral("panelTitles"), panelTitlesObject);
    root.insert(QStringLiteral("builtWorkbenches"), builtWorkbenches);
    root.insert(QStringLiteral("quickAccessIncluded"), includeQuickAccess);

    QFile outputFile(outputPath);
    const QFileInfo outputInfo(outputFile);
    QDir().mkpath(outputInfo.absolutePath());
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setLastError(QStringLiteral("Unable to write structure metadata cache: %1").arg(outputPath));
        return false;
    }
    outputFile.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    outputFile.close();
    return true;
}

// Returns the command-ID set for a toolbar panel object (keys of the "commands" sub-object,
// excluding separator tokens).
static QSet<QString> toolbarCommandSet(const QJsonObject& panelObject)
{
    QSet<QString> result;
    const auto orderArray = panelObject.value(QStringLiteral("order")).toArray();
    for (const auto& value : orderArray) {
        const QString id = value.toString().trimmed();
        if (!id.isEmpty()
            && !id.contains(QStringLiteral("_separator_"))
            && !id.endsWith(QStringLiteral("_separator"))) {
            result.insert(id);
        }
    }
    if (!result.isEmpty()) {
        return result;
    }

    const auto commandsObj = panelObject.value(QStringLiteral("commands")).toObject();
    for (auto it = commandsObj.begin(); it != commandsObj.end(); ++it) {
        const QString id = it.key().trimmed();
        if (!id.isEmpty()
            && !id.contains(QStringLiteral("_separator_"))
            && !id.endsWith(QStringLiteral("_separator"))) {
            result.insert(id);
        }
    }
    return result;
}

static int toolbarCommandPayloadCount(const QJsonObject& panelObject)
{
    int count = 0;
    const auto commandsObj = panelObject.value(QStringLiteral("commands")).toObject();
    for (auto it = commandsObj.begin(); it != commandsObj.end(); ++it) {
        const QString id = it.key().trimmed();
        if (!id.isEmpty()
            && !id.contains(QStringLiteral("_separator_"))
            && !id.endsWith(QStringLiteral("_separator"))) {
            ++count;
        }
    }
    return count;
}

static QString normalizedToolbarDedupKey(const QString& panelId, const QJsonObject& panelObject)
{
    QString label = panelObject.value(QStringLiteral("title")).toString(panelId);
    if (label.trimmed().isEmpty()) {
        label = panelId;
    }
    label.replace(QStringLiteral("&"), QString());
    label.replace(QLatin1Char('_'), QLatin1Char(' '));
    label.replace(QLatin1Char('-'), QLatin1Char(' '));
    label = label.simplified().toLower();

    QString compact;
    compact.reserve(label.size());
    for (const QChar ch : label) {
        compact.append(ch.isLetterOrNumber() ? ch : QLatin1Char(' '));
    }
    return compact.simplified();
}

static int toolbarSetIntersectionCount(const QSet<QString>& first, const QSet<QString>& second)
{
    int count = 0;
    for (const auto& item : first) {
        if (second.contains(item)) {
            ++count;
        }
    }
    return count;
}

static bool toolbarPanelsLikelyDuplicate(
    const QString& firstKey,
    const QSet<QString>& firstSet,
    const QString& secondKey,
    const QSet<QString>& secondSet
)
{
    if (firstSet.isEmpty() || secondSet.isEmpty()) {
        return false;
    }
    if (firstSet == secondSet) {
        return true;
    }

    const int intersection = toolbarSetIntersectionCount(firstSet, secondSet);
    if (intersection <= 0) {
        return false;
    }

    const qsizetype minSize = std::min(firstSet.size(), secondSet.size());
    const double coverage = static_cast<double>(intersection) / static_cast<double>(minSize);
    if (firstKey == secondKey && coverage >= 0.85) {
        return true;
    }
    return false;
}

QStringList orderedToolbarIds(const QJsonObject& toolbarsObject)
{
    QStringList ids;
    QSet<QString> present;
    struct ToolbarDedupEntry {
        QString id;
        QSet<QString> commandSet;
        QString normalizedKey;
        int payloadCommandCount = 0;
    };
    QVector<ToolbarDedupEntry> keptEntries;

    auto considerToolbar = [&](const QString& panelId) {
        const QString id = panelId.trimmed();
        if (id.isEmpty() || id == QStringLiteral("order") || !toolbarsObject.contains(id)) {
            return;
        }

        const QJsonObject panelObject = toolbarsObject.value(id).toObject();
        const QSet<QString> commandSet = toolbarCommandSet(panelObject);
        const QString normalizedKey = normalizedToolbarDedupKey(id, panelObject);
        const int payloadCommandCount = toolbarCommandPayloadCount(panelObject);

        for (int index = 0; index < keptEntries.size(); ++index) {
            auto& existing = keptEntries[index];
            if (!toolbarPanelsLikelyDuplicate(
                    existing.normalizedKey,
                    existing.commandSet,
                    normalizedKey,
                    commandSet)) {
                continue;
            }

            bool preferCandidate = commandSet.size() > existing.commandSet.size();
            if (!preferCandidate
                && commandSet.size() == existing.commandSet.size()
                && payloadCommandCount > existing.payloadCommandCount) {
                preferCandidate = true;
            }
            if (preferCandidate) {
                const qsizetype idIndex = ids.indexOf(existing.id);
                if (idIndex >= 0) {
                    ids[idIndex] = id;
                }
                present.remove(existing.id);
                existing.id = id;
                existing.commandSet = commandSet;
                existing.normalizedKey = normalizedKey;
                existing.payloadCommandCount = payloadCommandCount;
                present.insert(id);
            }
            return;
        }

        if (present.contains(id)) {
            return;
        }
        ids.push_back(id);
        present.insert(id);
        keptEntries.push_back(ToolbarDedupEntry{id, commandSet, normalizedKey, payloadCommandCount});
    };

    // Prefer explicit order first, but collapse duplicates found in that list.
    const auto orderArray = toolbarsObject.value(QStringLiteral("order")).toArray();
    for (const auto& value : orderArray) {
        considerToolbar(value.toString());
    }

    // Append remaining entries only when they are not duplicates of already kept
    // ordered panels, or when they are a richer replacement.
    for (auto it = toolbarsObject.begin(); it != toolbarsObject.end(); ++it) {
        if (it.key() == QStringLiteral("order")) {
            continue;
        }
        considerToolbar(it.key());
    }

    return ids;
}

QString normalizeDisplayTextForLookup(QString value)
{
    value = value.trimmed();
    if (value.isEmpty()) {
        return QString();
    }

    const qsizetype tabIndex = value.indexOf(QLatin1Char('\t'));
    if (tabIndex >= 0) {
        value = value.left(tabIndex);
    }
    value.replace(QStringLiteral("&"), QString());
    value.replace(QLatin1Char('_'), QLatin1Char(' '));
    value.replace(QLatin1Char('-'), QLatin1Char(' '));

    const QString decomposed = value.normalized(QString::NormalizationForm_D);
    QString normalized;
    normalized.reserve(decomposed.size());
    for (const QChar ch : decomposed) {
        const auto category = ch.category();
        if (
            category == QChar::Mark_NonSpacing
            || category == QChar::Mark_SpacingCombining
            || category == QChar::Mark_Enclosing
        ) {
            continue;
        }
        normalized.append(ch.isLetterOrNumber() ? ch.toLower() : QLatin1Char(' '));
    }
    return normalized.simplified();
}

QString canonicalCommandIdFromToken(const QString& token)
{
    const QString candidate = normalizeActionCandidate(token);
    if (candidate.isEmpty()) {
        return QString();
    }
    if (isSeparatorCommand(candidate) || looksLikeCommandId(candidate)) {
        return candidate;
    }
    return QString();
}

bool isAmbiguousDisplayText(
    const QHash<QString, QVector<QString>>& commandIdsByDisplayText,
    const QString& normalizedDisplayText
)
{
    const auto it = commandIdsByDisplayText.constFind(normalizedDisplayText);
    return it != commandIdsByDisplayText.constEnd() && it.value().size() > 1;
}

QString resolveCanonicalCommandIdSafely(
    const QString& token,
    const QJsonObject& commandsObject,
    const QHash<QString, QVector<QString>>& commandIdsByDisplayText
)
{
    const QString normalizedToken = normalizeActionCandidate(token);
    if (normalizedToken.isEmpty()) {
        return QString();
    }
    if (isSeparatorCommand(normalizedToken)) {
        return normalizedToken;
    }

    const QString directCanonicalId = canonicalCommandIdFromToken(normalizedToken);
    if (!directCanonicalId.isEmpty() && commandsObject.contains(directCanonicalId)) {
        return directCanonicalId;
    }

    QString displayLookupKey;
    if (commandsObject.contains(normalizedToken)) {
        displayLookupKey = normalizeDisplayTextForLookup(
            commandsObject.value(normalizedToken).toObject().value(QStringLiteral("text")).toString()
        );
    }
    if (displayLookupKey.isEmpty()) {
        displayLookupKey = normalizeDisplayTextForLookup(normalizedToken);
    }
    if (displayLookupKey.isEmpty()) {
        return QString();
    }

    const auto mappingIt = commandIdsByDisplayText.constFind(displayLookupKey);
    if (mappingIt == commandIdsByDisplayText.constEnd() || mappingIt.value().isEmpty()) {
        return QString();
    }
    if (isAmbiguousDisplayText(commandIdsByDisplayText, displayLookupKey)) {
        commandtabDebugLog(
            QStringLiteral("ambiguous-command-text"),
            QStringLiteral("Ignored ambiguous token '%1' (%2)")
                .arg(token, mappingIt.value().join(QStringLiteral(", ")))
        );
        return QString();
    }
    return mappingIt.value().first();
}

QStringList orderedCommandIdsFromPanelObject(const QJsonObject& panelObject)
{
    QStringList ids;
    QSet<QString> seen;
    const auto commandsObject = panelObject.value(QStringLiteral("commands")).toObject();

    QHash<QString, QVector<QString>> commandIdsByText;
    auto appendUniqueId = [](QVector<QString>* values, const QString& id) {
        if (values == nullptr || id.isEmpty() || values->contains(id)) {
            return;
        }
        values->push_back(id);
    };
    auto registerTextCandidate = [&](const QString& commandKey, const QJsonObject& commandPayload) {
        const QString canonicalCommandId = canonicalCommandIdFromToken(commandKey);
        if (canonicalCommandId.isEmpty()) {
            return;
        }

        const QString textKey = normalizeDisplayTextForLookup(
            commandPayload.value(QStringLiteral("text")).toString()
        );
        if (textKey.isEmpty()) {
            return;
        }
        appendUniqueId(&commandIdsByText[textKey], canonicalCommandId);
    };

    for (auto it = commandsObject.begin(); it != commandsObject.end(); ++it) {
        if (!it.value().isObject()) {
            continue;
        }
        registerTextCandidate(it.key(), it.value().toObject());
    }

    const auto orderArray = panelObject.value(QStringLiteral("order")).toArray();
    for (const auto& value : orderArray) {
        const QString resolvedId = resolveCanonicalCommandIdSafely(
            value.toString(),
            commandsObject,
            commandIdsByText
        );
        if (resolvedId.isEmpty() || seen.contains(resolvedId)) {
            continue;
        }
        ids.push_back(resolvedId);
        seen.insert(resolvedId);
    }

    for (auto it = commandsObject.begin(); it != commandsObject.end(); ++it) {
        const QString resolvedId = resolveCanonicalCommandIdSafely(
            it.key(),
            commandsObject,
            commandIdsByText
        );
        if (resolvedId.isEmpty() || seen.contains(resolvedId)) {
            continue;
        }
        ids.push_back(resolvedId);
        seen.insert(resolvedId);
    }
    return ids;
}

QStringList orderedCommandIds(const QJsonObject& panelObject)
{
    return orderedCommandIdsFromPanelObject(panelObject);
}

QSet<QString> ignoredWorkbenchTitles(const QJsonObject& structureRoot)
{
    QSet<QString> ignored;
    const auto array = structureRoot.value(QStringLiteral("ignoredWorkbenches")).toArray();
    for (const auto& value : array) {
        const QString item = value.toString().trimmed().toLower();
        if (!item.isEmpty()) {
            ignored.insert(item);
        }
    }
    return ignored;
}

QSet<QString> ignoredToolbarTitles(const QJsonObject& structureRoot)
{
    QSet<QString> ignored;
    const auto array = structureRoot.value(QStringLiteral("ignoredToolbars")).toArray();
    for (const auto& value : array) {
        const QString item = value.toString().trimmed().toLower();
        if (!item.isEmpty()) {
            ignored.insert(item);
        }
    }
    return ignored;
}

bool isIgnoredWorkbench(const QJsonObject& structureRoot, const QString& workbenchId)
{
    const auto ignored = ignoredWorkbenchTitles(structureRoot);
    return ignored.contains(workbenchId.trimmed().toLower());
}

bool isIgnoredToolbar(
    const QJsonObject& structureRoot,
    const QString& workbenchId,
    const QString& panelId,
    const QJsonObject& panelObject
)
{
    const auto ignored = ignoredToolbarTitles(structureRoot);
    const QString normalizedWorkbenchId = workbenchId.trimmed().toLower();
    const QString normalizedPanelId = panelId.trimmed().toLower();
    if (!normalizedWorkbenchId.isEmpty() && !normalizedPanelId.isEmpty()) {
        if (ignored.contains(normalizedWorkbenchId + QStringLiteral("::") + normalizedPanelId)) {
            return true;
        }
    }
    if (ignored.contains(normalizedPanelId)) {
        return true;
    }
    const QString title = panelObject.value(QStringLiteral("title")).toString(panelId).trimmed().toLower();
    if (!normalizedWorkbenchId.isEmpty() && !title.isEmpty()) {
        if (ignored.contains(normalizedWorkbenchId + QStringLiteral("::") + title)) {
            return true;
        }
    }
    return ignored.contains(title);
}

QString panelTitleFromId(const QString& panelId, const QString& suffix)
{
    QString title = panelId;
    if (!suffix.isEmpty() && title.endsWith(suffix)) {
        title.chop(suffix.size());
    }
    title.replace(QLatin1Char('_'), QLatin1Char(' '));
    title.replace(QLatin1Char('-'), QLatin1Char(' '));
    title.replace(QStringLiteral("&"), QString());

    QString spaced;
    spaced.reserve(title.size() + 8);
    for (int index = 0; index < title.size(); ++index) {
        const QChar current = title.at(index);
        if (
            index > 0
            && current.isUpper()
            && title.at(index - 1).isLetterOrNumber()
            && title.at(index - 1).isLower()
        ) {
            spaced.append(QLatin1Char(' '));
        }
        spaced.append(current);
    }
    spaced = spaced.simplified();
    if (spaced.isEmpty()) {
        return panelId.trimmed();
    }

    QString titleCase;
    titleCase.reserve(spaced.size());
    bool newWord = true;
    for (const QChar ch : spaced) {
        if (ch.isSpace()) {
            newWord = true;
            titleCase.append(ch);
            continue;
        }
        titleCase.append(newWord ? ch.toUpper() : ch);
        newWord = false;
    }
    return titleCase;
}

QString normalizedPanelTitleKey(const QString& panelTitle)
{
    const QString normalized = normalizeDisplayTextForLookup(panelTitle);
    if (normalized.isEmpty()) {
        return QString();
    }

    const QSet<QString> ignoredTokens = {
        QStringLiteral("toolbar"),
        QStringLiteral("panel"),
        QStringLiteral("tool"),
        QStringLiteral("tools"),
        QStringLiteral("outil"),
        QStringLiteral("outils"),
        QStringLiteral("workbench"),
        QStringLiteral("de"),
        QStringLiteral("des"),
        QStringLiteral("du"),
        QStringLiteral("la"),
        QStringLiteral("le"),
        QStringLiteral("les"),
        QStringLiteral("the"),
        QStringLiteral("and"),
        QStringLiteral("et"),
    };
    const QHash<QString, QString> aliases = {
        {QStringLiteral("booleen"), QStringLiteral("bool")},
        {QStringLiteral("booleenne"), QStringLiteral("bool")},
        {QStringLiteral("booleens"), QStringLiteral("bool")},
        {QStringLiteral("booleennes"), QStringLiteral("bool")},
        {QStringLiteral("boolean"), QStringLiteral("bool")},
        {QStringLiteral("maillage"), QStringLiteral("mesh")},
        {QStringLiteral("analyse"), QStringLiteral("analyze")},
        {QStringLiteral("analyze"), QStringLiteral("analyze")},
        {QStringLiteral("evaluation"), QStringLiteral("evaluate")},
        {QStringLiteral("evaluer"), QStringLiteral("evaluate")},
        {QStringLiteral("reparer"), QStringLiteral("repair")},
        {QStringLiteral("repare"), QStringLiteral("repair")},
    };

    QStringList normalizedTokens;
    const QStringList tokens = normalized.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (const auto& token : tokens) {
        if (ignoredTokens.contains(token)) {
            continue;
        }
        const auto aliasIt = aliases.constFind(token);
        normalizedTokens.push_back(aliasIt != aliases.constEnd() ? aliasIt.value() : token);
    }
    if (normalizedTokens.isEmpty()) {
        return normalized;
    }
    return normalizedTokens.join(QLatin1Char(' '));
}

QVector<CommandTabCommandEntry> deduplicateCommands(const QVector<CommandTabCommandEntry>& commands)
{
    QVector<CommandTabCommandEntry> deduplicated;
    QSet<QString> seenCommandIds;
    bool previousWasSeparator = true;

    for (auto command : commands) {
        if (command.type == QStringLiteral("separator") || isSeparatorCommand(command.id)) {
            if (!deduplicated.isEmpty() && !previousWasSeparator) {
                deduplicated.push_back(command);
                previousWasSeparator = true;
            }
            continue;
        }

        command.menuCommands = deduplicateCommands(command.menuCommands);
        QString canonicalId = canonicalCommandIdFromToken(command.id);
        if (canonicalId.isEmpty()) {
            canonicalId = normalizeActionCandidate(command.id);
        }
        if (canonicalId.isEmpty()) {
            continue;
        }

        const QString dedupKey = canonicalId.toLower();
        if (seenCommandIds.contains(dedupKey)) {
            commandtabDebugLog(
                QStringLiteral("command-dedup"),
                QStringLiteral("Dropped duplicate command '%1'").arg(canonicalId)
            );
            continue;
        }

        command.id = canonicalId;
        deduplicated.push_back(command);
        seenCommandIds.insert(dedupKey);
        previousWasSeparator = false;
    }

    while (!deduplicated.isEmpty()) {
        const auto& tail = deduplicated.back();
        if (tail.type != QStringLiteral("separator") && !isSeparatorCommand(tail.id)) {
            break;
        }
        deduplicated.pop_back();
    }
    return deduplicated;
}

QSet<QString> panelCommandIdSet(const QVector<CommandTabCommandEntry>& commands)
{
    QSet<QString> ids;
    for (const auto& command : commands) {
        if (command.type == QStringLiteral("separator") || isSeparatorCommand(command.id)) {
            continue;
        }
        const QString canonicalId = canonicalCommandIdFromToken(command.id);
        if (!canonicalId.isEmpty()) {
            ids.insert(canonicalId.toLower());
        }
    }
    return ids;
}

QString panelCommandSignature(const QVector<CommandTabCommandEntry>& commands)
{
    QStringList normalizedIds = panelCommandIdSet(commands).values();
    std::sort(normalizedIds.begin(), normalizedIds.end());
    return normalizedIds.join(QStringLiteral("|"));
}

QString buildPanelDedupSignature(
    const QString& sourceWorkbenchId,
    const QString& panelTitle,
    const QVector<CommandTabCommandEntry>& commands,
    const QString& sourceType
)
{
    return QStringLiteral("%1|%2|%3|%4")
        .arg(sourceWorkbenchId.trimmed().toLower(),
             normalizedPanelTitleKey(panelTitle),
             panelCommandSignature(commands),
             sourceType.trimmed().toLower());
}

QStringList workbenchIconCandidates(const QString& workbenchId, const QString& workbenchTitle)
{
    QStringList candidates;
    auto appendCandidate = [&candidates](const QString& value) {
        const QString candidate = value.trimmed();
        if (!candidate.isEmpty() && !candidates.contains(candidate)) {
            candidates.push_back(candidate);
        }
    };
    auto appendIconCandidateVariants = [&appendCandidate](const QString& value) {
        const QString candidate = value.trimmed();
        if (candidate.isEmpty()) {
            return;
        }
        appendCandidate(candidate);

        const QString baseName = QFileInfo(candidate).completeBaseName();
        if (baseName.isEmpty()) {
            return;
        }
        appendCandidate(QStringLiteral(":/icons/%1.svg").arg(baseName));
        appendCandidate(QStringLiteral(":/icons/%1.png").arg(baseName));
        appendCandidate(QStringLiteral("icons:%1").arg(baseName));
        appendCandidate(QStringLiteral(":/icons/Workbench_%1.svg").arg(baseName));
        appendCandidate(QStringLiteral(":/icons/Workbench_%1.png").arg(baseName));
        appendCandidate(QStringLiteral("icons:Workbench_%1").arg(baseName));
    };
    auto appendCompoundVariants = [&appendIconCandidateVariants](const QString& value) {
        const QString token = value.trimmed();
        if (token.isEmpty()) {
            return;
        }

        QString compact = token;
        compact.remove(QLatin1Char(' '));
        const QString underscored = compact;
        const QString dashed = QString(compact).replace(QLatin1Char('_'), QLatin1Char('-'));
        const QString lowered = compact.toLower();
        const QString loweredUnderscored = QString(underscored).toLower();
        const QString loweredDashed = QString(dashed).toLower();

        appendIconCandidateVariants(compact);
        appendIconCandidateVariants(underscored);
        appendIconCandidateVariants(dashed);
        appendIconCandidateVariants(lowered);
        appendIconCandidateVariants(loweredUnderscored);
        appendIconCandidateVariants(loweredDashed);
    };

    appendIconCandidateVariants(workbenchId);
    if (workbenchId.endsWith(QStringLiteral("Workbench")) && workbenchId.size() > 9) {
        const QString baseWorkbenchId = workbenchId.left(workbenchId.size() - 9);
        appendIconCandidateVariants(baseWorkbenchId);
        appendCompoundVariants(baseWorkbenchId);
    }
    appendCompoundVariants(workbenchId);

    QString title = workbenchTitle;
    title.replace(QStringLiteral("&"), QString());
    title = title.trimmed();
    appendIconCandidateVariants(title);
    QString compactTitle = title;
    appendIconCandidateVariants(compactTitle.replace(QLatin1Char(' '), QString()));
    QString underscoredTitle = title;
    appendIconCandidateVariants(underscoredTitle.replace(QLatin1Char(' '), QLatin1Char('_')));
    QString dashedTitle = title;
    appendIconCandidateVariants(dashedTitle.replace(QLatin1Char(' '), QLatin1Char('-')));
    appendCompoundVariants(title);

    const QString normalizedWorkbenchId = workbenchId.trimmed().toLower();
    if (normalizedWorkbenchId == QStringLiteral("inspectionworkbench")) {
        appendIconCandidateVariants(QStringLiteral("inspection"));
    } else if (normalizedWorkbenchId == QStringLiteral("archworkbench")) {
        appendIconCandidateVariants(QStringLiteral("BIMWorkbench"));
        appendIconCandidateVariants(QStringLiteral("Arch_Building"));
    } else if (
        normalizedWorkbenchId == QStringLiteral("camworkbench")
        || normalizedWorkbenchId == QStringLiteral("pathworkbench")
    ) {
        appendIconCandidateVariants(QStringLiteral("CAM_Job"));
        appendIconCandidateVariants(QStringLiteral("CAM_Toolpath"));
        appendIconCandidateVariants(QStringLiteral("PathWorkbench"));
    } else if (normalizedWorkbenchId == QStringLiteral("openscadworkbench")) {
        appendIconCandidateVariants(QStringLiteral("openscad"));
        appendIconCandidateVariants(QStringLiteral("preferences-openscad"));
    } else if (normalizedWorkbenchId == QStringLiteral("pointsworkbench")) {
        appendIconCandidateVariants(QStringLiteral("points"));
    } else if (normalizedWorkbenchId == QStringLiteral("reverseengineeringworkbench")) {
        appendIconCandidateVariants(QStringLiteral("reverseengineering"));
        appendIconCandidateVariants(QStringLiteral("reverse_engineering"));
    } else if (normalizedWorkbenchId == QStringLiteral("startworkbench")) {
        appendIconCandidateVariants(QStringLiteral("StartCommandIcon"));
        appendIconCandidateVariants(QStringLiteral("preferences-start"));
    } else if (normalizedWorkbenchId == QStringLiteral("techdrawworkbench")) {
        appendIconCandidateVariants(QStringLiteral("TechDraw_PageDefault"));
        appendIconCandidateVariants(QStringLiteral("TechDraw_View"));
    } else if (normalizedWorkbenchId == QStringLiteral("measureworkbench")) {
        appendIconCandidateVariants(QStringLiteral("Measurement-Distance"));
        appendIconCandidateVariants(QStringLiteral("Measurement-Group"));
    } else if (normalizedWorkbenchId == QStringLiteral("webworkbench")) {
        appendIconCandidateVariants(QStringLiteral("internet-web-browser"));
        appendIconCandidateVariants(QStringLiteral("help-browser"));
    }

    return candidates;
}

QString resolveWorkbenchIconSource(
    const QString& addonRootPath,
    const QString& workbenchId,
    const QString& workbenchTitle
)
{
    for (const auto& candidate : workbenchIconCandidates(workbenchId, workbenchTitle)) {
        QString iconSource = directlyLoadableIconSource(candidate);
        if (iconSource.isEmpty()) {
            iconSource = resolveIconPath(addonRootPath, candidate);
        }
        if (!iconSource.isEmpty()) {
            return iconSource;
        }
        const QIcon themedIcon = loadIconFromSource(candidate);
        if (!themedIcon.isNull()) {
            return candidate;
        }
    }
    return QString();
}

QJsonArray dropdownDefinition(const QJsonObject& structureRoot, const QString& commandId)
{
    return structureRoot.value(QStringLiteral("dropdownButtons")).toObject().value(commandId).toArray();
}

void collectDropdownMetadataRequirements(
    const QJsonObject& structureRoot,
    const QString& commandId,
    QSet<QString>* commandIds,
    QHash<QString, QJsonObject>* commandDefinitions
)
{
    if (commandIds == nullptr) {
        return;
    }
    const auto dropdownArray = dropdownDefinition(structureRoot, commandId);
    for (const auto& entryValue : dropdownArray) {
        if (!entryValue.isArray()) {
            continue;
        }
        const auto entryArray = entryValue.toArray();
        if (entryArray.isEmpty()) {
            continue;
        }
        const QString nestedCommandId = entryArray.at(0).toString().trimmed();
        if (nestedCommandId.isEmpty() || isSeparatorCommand(nestedCommandId)) {
            continue;
        }
        commandIds->insert(nestedCommandId);
        mergeStructureCommandDefinition(commandDefinitions, nestedCommandId, QJsonObject());
    }
}

bool workbenchHasEnabledPanels(
    const QJsonObject& structureRoot,
    const QString& workbenchId,
    const QJsonObject& workbenchObject
)
{
    if (isIgnoredWorkbench(structureRoot, workbenchId)) {
        return false;
    }

    const auto toolbarsObject = workbenchObject.value(QStringLiteral("toolbars")).toObject();
    for (const auto& toolbarId : orderedToolbarIds(toolbarsObject)) {
        const auto panelObject = toolbarsObject.value(toolbarId).toObject();
        if (isIgnoredToolbar(structureRoot, workbenchId, toolbarId, panelObject)) {
            continue;
        }
        if (panelObject.value(QStringLiteral("Enabled")).toBool(true) && !orderedCommandIds(panelObject).isEmpty()) {
            return true;
        }
    }

    const auto customPanelsObject = structureRoot.value(QStringLiteral("customToolbars")).toObject().value(workbenchId).toObject();
    for (auto it = customPanelsObject.begin(); it != customPanelsObject.end(); ++it) {
        if (!it.value().isObject()) {
            continue;
        }
        if (isIgnoredToolbar(structureRoot, workbenchId, it.key(), it.value().toObject())) {
            continue;
        }
        if (!it.value().toObject().value(QStringLiteral("commands")).toObject().isEmpty()) {
            return true;
        }
    }

    const auto newPanelsRoot = structureRoot.value(QStringLiteral("newPanels")).toObject();
    for (const QString& source : {workbenchId, QStringLiteral("Global")}) {
        const auto newPanelsObject = newPanelsRoot.value(source).toObject();
        for (auto it = newPanelsObject.begin(); it != newPanelsObject.end(); ++it) {
            if (isIgnoredToolbar(structureRoot, source, it.key(), QJsonObject())) {
                continue;
            }
            if (it.value().isArray() && !it.value().toArray().isEmpty()) {
                return true;
            }
        }
    }
    return false;
}

CommandTabCommandEntry buildStructureCommand(
    const QJsonObject& structureRoot,
    const QString& commandId,
    const QJsonObject& commandObject,
    const CommandTabMetadataCache& metadata,
    const CommandTabSettingsState& settings,
    const QString& addonRootPath,
    const QString& sourceWorkbenchId = QString(),
    const QString& sourceToolbarTitle = QString()
)
{
    CommandTabCommandEntry command;
    if (isSeparatorCommand(commandId)) {
        command.type = QStringLiteral("separator");
        command.id = commandId;
        return command;
    }

    const bool isDropdown = commandId.endsWith(QStringLiteral("_ddb"));
    command.type = isDropdown ? QStringLiteral("dropdown") : QStringLiteral("command");
    command.id = commandId;
    command.size = commandObject.value(QStringLiteral("size")).toString(QStringLiteral("small")).toLower();
    command.sourceWorkbenchId = sourceWorkbenchId;
    command.sourceToolbarTitle = sourceToolbarTitle;
    if (
        command.size != QStringLiteral("large")
        && command.size != QStringLiteral("medium")
        && command.size != QStringLiteral("small")
    ) {
        command.size = QStringLiteral("small");
    }

    const auto metadataIt = metadata.commands.constFind(commandId);
    if (metadataIt != metadata.commands.constEnd()) {
        command.text = metadataIt->text;
    }
    if (looksLikeTechnicalCommandText(command.text, commandId)) {
        command.text.clear();
    }
    if (command.text.isEmpty()) {
        command.text = commandObject.value(QStringLiteral("text")).toString();
    }
    if (command.text.isEmpty()) {
        command.text = isDropdown ? panelTitleFromId(commandId, QStringLiteral("_ddb")) : commandId;
    }
    command.text = correctedCommandDisplayText(commandId, command.text);
    bool explicitTextVisibility = false;
    command.textVisible = resolvedTextVisibilityForCommand(
        settings,
        commandObject,
        command.size,
        &explicitTextVisibility
    );
    command.textVisibilityExplicit = explicitTextVisibility;

    if (metadataIt != metadata.commands.constEnd()) {
        command.iconPath = metadataIt->iconPath;
    }
    int minimumCommandIconEdge = 0;
    if (command.size == QStringLiteral("large")) {
        minimumCommandIconEdge = 28;
    } else if (command.size == QStringLiteral("medium")) {
        minimumCommandIconEdge = 20;
    }
    if (
        minimumCommandIconEdge > 0
        && isLikelyLowResolutionIconFile(command.iconPath, minimumCommandIconEdge)
    ) {
        command.iconPath.clear();
    }
    if (command.iconPath.isEmpty()) {
        const QString iconHint = commandObject.value(QStringLiteral("icon")).toString().trimmed();
        if (!iconHint.isEmpty()) {
            command.iconPath = directlyLoadableIconSource(iconHint);
            if (command.iconPath.isEmpty()) {
                command.iconPath = resolveIconPath(addonRootPath, iconHint);
            }
        }
    }
    if (command.iconPath.isEmpty()) {
        for (const auto& candidate : commandIconCandidates(commandId)) {
            command.iconPath = directlyLoadableIconSource(candidate);
            if (command.iconPath.isEmpty()) {
                command.iconPath = resolveIconPath(addonRootPath, candidate);
            }
            if (!command.iconPath.isEmpty()) {
                break;
            }
        }
    }
    if (isDropdown) {
        const auto dropdownArray = dropdownDefinition(structureRoot, commandId);
        for (const auto& entryValue : dropdownArray) {
            if (!entryValue.isArray()) {
                continue;
            }
            const auto entryArray = entryValue.toArray();
            if (entryArray.isEmpty()) {
                continue;
            }
            const QString nestedCommandId = entryArray.at(0).toString().trimmed();
            if (nestedCommandId.isEmpty()) {
                continue;
            }
            const QString nestedSourceWorkbenchId = entryArray.size() > 1
                ? entryArray.at(1).toString().trimmed()
                : sourceWorkbenchId;
            CommandTabCommandEntry nestedCommand =
                buildStructureCommand(
                    structureRoot,
                    nestedCommandId,
                    QJsonObject(),
                    metadata,
                    settings,
                    addonRootPath,
                    nestedSourceWorkbenchId,
                    QString()
                );
            if (!nestedCommand.id.isEmpty()) {
                command.menuCommands.push_back(nestedCommand);
                if (command.iconPath.isEmpty()) {
                    command.iconPath = nestedCommand.iconPath;
                }
            }
        }
    }
    return command;
}

CommandTabPanelEntry buildPanelFromStructure(
    const QJsonObject& structureRoot,
    const QString& workbenchId,
    const QString& panelId,
    const QJsonObject& panelObject,
    const CommandTabMetadataCache& metadata,
    const CommandTabSettingsState& settings,
    const QString& addonRootPath
)
{
    CommandTabPanelEntry panel;
    panel.id = panelId;
    const QString localizedTitle = metadata.panelTitles.value(panelId).trimmed();
    if (!localizedTitle.isEmpty()) {
        panel.title = localizedTitle;
    } else {
        const QString panelTitle = panelObject.value(QStringLiteral("title")).toString(panelId);
        panel.title = panelTitleFromId(panelTitle.isEmpty() ? panelId : panelTitle, QString());
    }
    panel.sourceType = QStringLiteral("toolbar");
    panel.sourceWorkbenchId = workbenchId;

    const auto commandsObject = panelObject.value(QStringLiteral("commands")).toObject();
    for (const auto& commandId : orderedCommandIds(panelObject)) {
        const auto commandObject = commandsObject.value(commandId).toObject();
        panel.commands.push_back(
            buildStructureCommand(
                structureRoot,
                commandId,
                commandObject,
                metadata,
                settings,
                addonRootPath,
                workbenchId,
                panel.title
            )
        );
    }
    panel.commands = deduplicateCommands(panel.commands);
    return panel;
}

CommandTabWorkbenchEntry buildWorkbenchFromStructure(
    const QJsonObject& structureRoot,
    const QString& workbenchId,
    const QJsonObject& workbenchObject,
    const CommandTabMetadataCache& metadata,
    const CommandTabSettingsState& settings,
    const QString& addonRootPath
)
{
    CommandTabWorkbenchEntry workbench;
    workbench.id = workbenchId;
    workbench.title = cleanedWorkbenchTitle(metadata.workbenchTitles.value(workbenchId, workbenchId), workbenchId);
    workbench.iconPath = metadata.workbenchIcons.value(workbenchId);
    if (workbench.iconPath.isEmpty()) {
        workbench.iconPath = resolveWorkbenchIconSource(addonRootPath, workbenchId, workbench.title);
    }

    struct PanelBuildCandidate {
        CommandTabPanelEntry panel;
        int priority = 0;
        int sourceOrder = 0;
        bool fromGlobal = false;
        QString normalizedTitle;
        QString commandSignature;
        QString dedupSignature;
        QSet<QString> commandSet;
    };

    auto overlapCoverage = [](const QSet<QString>& first, const QSet<QString>& second) -> double {
        if (first.isEmpty() || second.isEmpty()) {
            return 0.0;
        }
        int intersection = 0;
        for (const auto& item : first) {
            if (second.contains(item)) {
                ++intersection;
            }
        }
        const qsizetype minimumSize = std::min(first.size(), second.size());
        if (minimumSize <= 0) {
            return 0.0;
        }
        return static_cast<double>(intersection) / static_cast<double>(minimumSize);
    };

    auto isBetterCandidate = [](const PanelBuildCandidate& candidate, const PanelBuildCandidate& existing) {
        if (candidate.priority != existing.priority) {
            return candidate.priority > existing.priority;
        }
        if (candidate.commandSet.size() != existing.commandSet.size()) {
            return candidate.commandSet.size() > existing.commandSet.size();
        }
        return candidate.sourceOrder < existing.sourceOrder;
    };

    QVector<PanelBuildCandidate> candidates;
    candidates.reserve(48);
    int sourceOrder = 0;

    auto appendCandidate = [&](CommandTabPanelEntry panel, int priority, bool fromGlobal) {
        panel.commands = deduplicateCommands(panel.commands);
        if (panel.commands.isEmpty()) {
            return;
        }

        PanelBuildCandidate candidate;
        candidate.panel = std::move(panel);
        candidate.priority = priority;
        candidate.sourceOrder = sourceOrder++;
        candidate.fromGlobal = fromGlobal;
        candidate.normalizedTitle = normalizedPanelTitleKey(candidate.panel.title);
        candidate.commandSet = panelCommandIdSet(candidate.panel.commands);
        candidate.commandSignature = panelCommandSignature(candidate.panel.commands);
        candidate.dedupSignature = buildPanelDedupSignature(
            candidate.panel.sourceWorkbenchId,
            candidate.panel.title,
            candidate.panel.commands,
            candidate.panel.sourceType
        );
        if (candidate.commandSet.isEmpty()) {
            return;
        }
        candidates.push_back(candidate);
    };

    const auto newPanelsRoot = structureRoot.value(QStringLiteral("newPanels")).toObject();
    QSet<QString> newPanelIdsForWorkbench;
    for (const QString& source : {workbenchId, QStringLiteral("Global")}) {
        const auto panelsObject = newPanelsRoot.value(source).toObject();
        for (auto it = panelsObject.begin(); it != panelsObject.end(); ++it) {
            const QString panelId = it.key().trimmed();
            if (!panelId.isEmpty()) {
                newPanelIdsForWorkbench.insert(panelId);
            }
        }
    }

    const auto toolbarsObject = workbenchObject.value(QStringLiteral("toolbars")).toObject();
    QSet<QString> explicitOrderedToolbarIds;
    for (const auto& value : toolbarsObject.value(QStringLiteral("order")).toArray()) {
        const QString panelId = value.toString().trimmed();
        if (!panelId.isEmpty()) {
            explicitOrderedToolbarIds.insert(panelId);
        }
    }

    for (const auto& panelId : orderedToolbarIds(toolbarsObject)) {
        const auto panelObject = toolbarsObject.value(panelId).toObject();
        if (isIgnoredToolbar(structureRoot, workbenchId, panelId, panelObject)) {
            continue;
        }
        if (panelObject.value(QStringLiteral("Enabled")).toBool(true) == false) {
            continue;
        }

        const bool isExplicitToolbar = explicitOrderedToolbarIds.contains(panelId);
        if (!isExplicitToolbar && newPanelIdsForWorkbench.contains(panelId)) {
            commandtabDebugLog(
                QStringLiteral("panel-dedup"),
                QStringLiteral("Skipped orphan toolbar '%1' due to new-panel collision").arg(panelId)
            );
            continue;
        }

        CommandTabPanelEntry panel = buildPanelFromStructure(
            structureRoot,
            workbenchId,
            panelId,
            panelObject,
            metadata,
            settings,
            addonRootPath
        );
        appendCandidate(std::move(panel), isExplicitToolbar ? 400 : 300, false);
    }

    const auto customPanelsObject = structureRoot.value(QStringLiteral("customToolbars")).toObject().value(workbenchId).toObject();
    for (auto it = customPanelsObject.begin(); it != customPanelsObject.end(); ++it) {
        if (!it.value().isObject()) {
            continue;
        }
        const QString panelId = it.key();
        if (isIgnoredToolbar(structureRoot, workbenchId, panelId, it.value().toObject())) {
            continue;
        }

        CommandTabPanelEntry panel;
        panel.id = panelId;
        const QString localizedTitle = metadata.panelTitles.value(panelId).trimmed();
        panel.title = localizedTitle.isEmpty() ? panelTitleFromId(panelId, QStringLiteral("_custom")) : localizedTitle;
        panel.sourceType = QStringLiteral("custom");
        panel.sourceWorkbenchId = workbenchId;
        const auto commandsMap = it.value().toObject().value(QStringLiteral("commands")).toObject();
        for (auto commandIt = commandsMap.begin(); commandIt != commandsMap.end(); ++commandIt) {
            panel.commands.push_back(
                buildStructureCommand(
                    structureRoot,
                    commandIt.key(),
                    QJsonObject(),
                    metadata,
                    settings,
                    addonRootPath,
                    workbenchId,
                    commandIt.value().toString()
                )
            );
        }
        appendCandidate(std::move(panel), 200, false);
    }

    for (const QString& source : {workbenchId, QStringLiteral("Global")}) {
        const auto newPanelsObject = newPanelsRoot.value(source).toObject();
        for (auto it = newPanelsObject.begin(); it != newPanelsObject.end(); ++it) {
            const QString panelId = it.key();
            if (isIgnoredToolbar(structureRoot, source, panelId, QJsonObject())) {
                continue;
            }
            if (!it.value().isArray()) {
                continue;
            }

            CommandTabPanelEntry panel;
            panel.id = panelId;
            const QString localizedTitle = metadata.panelTitles.value(panelId).trimmed();
            panel.title = localizedTitle.isEmpty() ? panelTitleFromId(panelId, QStringLiteral("_newPanel")) : localizedTitle;
            panel.sourceType = QStringLiteral("new");
            panel.sourceWorkbenchId = source;
            for (const auto& commandValue : it.value().toArray()) {
                if (!commandValue.isArray()) {
                    continue;
                }
                const auto commandArray = commandValue.toArray();
                if (commandArray.isEmpty()) {
                    continue;
                }
                const QString commandId = commandArray.at(0).toString().trimmed();
                if (commandId.isEmpty()) {
                    continue;
                }
                const QString commandSourceWorkbenchId = commandArray.size() > 1
                    ? commandArray.at(1).toString().trimmed()
                    : source;
                panel.commands.push_back(
                    buildStructureCommand(
                        structureRoot,
                        commandId,
                        QJsonObject(),
                        metadata,
                        settings,
                        addonRootPath,
                        commandSourceWorkbenchId,
                        QString()
                    )
                );
            }
            const bool isGlobal = source == QStringLiteral("Global");
            appendCandidate(std::move(panel), isGlobal ? 100 : 200, isGlobal);
        }
    }

    QVector<PanelBuildCandidate> deduplicatedPanels;
    deduplicatedPanels.reserve(candidates.size());
    for (const auto& candidate : candidates) {
        bool handled = false;
        for (auto& existing : deduplicatedPanels) {
            const bool exactDuplicate = !candidate.dedupSignature.isEmpty()
                && candidate.dedupSignature == existing.dedupSignature;
            const bool sameCommandSignature =
                !candidate.commandSignature.isEmpty()
                && candidate.commandSignature == existing.commandSignature;
            const double coverage = overlapCoverage(candidate.commandSet, existing.commandSet);
            const qsizetype minimumSize = std::min(candidate.commandSet.size(), existing.commandSet.size());
            const bool sameTitleFamily =
                !candidate.normalizedTitle.isEmpty()
                && candidate.normalizedTitle == existing.normalizedTitle;
            const bool commandSignatureDuplicate =
                sameCommandSignature && minimumSize >= 3;
            const bool strongCollision =
                minimumSize >= 3
                && coverage >= 0.85
                && (sameTitleFamily || candidate.fromGlobal || existing.fromGlobal);
            if (!exactDuplicate && !commandSignatureDuplicate && !strongCollision) {
                continue;
            }

            handled = true;
            if (isBetterCandidate(candidate, existing)) {
                commandtabDebugLog(
                    QStringLiteral("panel-dedup"),
                    QStringLiteral("Replacing panel '%1' with '%2'").arg(existing.panel.id, candidate.panel.id)
                );
                existing = candidate;
            } else if (strongCollision && !exactDuplicate) {
                QVector<CommandTabCommandEntry> mergedCommands = existing.panel.commands;
                mergedCommands += candidate.panel.commands;
                mergedCommands = deduplicateCommands(mergedCommands);
                const QSet<QString> mergedSet = panelCommandIdSet(mergedCommands);
                if (mergedSet.size() > existing.commandSet.size()) {
                    existing.panel.commands = mergedCommands;
                    existing.commandSet = mergedSet;
                    existing.commandSignature = panelCommandSignature(existing.panel.commands);
                }
            }
            break;
        }

        if (!handled) {
            deduplicatedPanels.push_back(candidate);
        }
    }

    for (const auto& candidate : deduplicatedPanels) {
        if (!candidate.panel.commands.isEmpty()) {
            workbench.panels.push_back(candidate.panel);
        }
    }
    return workbench;
}

QVector<CommandTabWorkbenchVisibilityEntry> buildWorkbenchVisibilityFromStructure(
    const QJsonObject& structureRoot,
    const CommandTabMetadataCache& metadata,
    const QString& addonRootPath
)
{
    QVector<CommandTabWorkbenchVisibilityEntry> entries;
    const auto ignored = ignoredWorkbenchTitles(structureRoot);
    const auto workbenchesObject = structureRoot.value(QStringLiteral("workbenches")).toObject();
    for (auto it = workbenchesObject.begin(); it != workbenchesObject.end(); ++it) {
        if (!it.value().isObject()) {
            continue;
        }
        CommandTabWorkbenchVisibilityEntry entry;
        entry.id = it.key();
        entry.title = cleanedWorkbenchTitle(
            metadata.workbenchTitles.value(entry.id, entry.id),
            entry.id
        );
        const QString normalizedWorkbenchId = entry.id.trimmed().toLower();
        const QString normalizedWorkbenchTitle = entry.title.trimmed().toLower();
        if (
            normalizedWorkbenchId == QStringLiteral("noneworkbench")
            || normalizedWorkbenchId == QStringLiteral("<none>")
            || normalizedWorkbenchId == QStringLiteral("none")
            || normalizedWorkbenchTitle == QStringLiteral("<none>")
            || normalizedWorkbenchTitle == QStringLiteral("none")
        ) {
            continue;
        }
        entry.iconPath = metadata.workbenchIcons.value(entry.id);
        if (entry.iconPath.isEmpty()) {
            entry.iconPath = resolveWorkbenchIconSource(addonRootPath, entry.id, entry.title);
        }
        const QString loweredId = entry.id.trimmed().toLower();
        const QString loweredTitle = entry.title.trimmed().toLower();
        entry.visible = !ignored.contains(loweredId) && !ignored.contains(loweredTitle);
        entries.push_back(entry);
    }
    return entries;
}

QVector<CommandTabPanelVisibilityEntry> buildPanelVisibilityFromStructure(
    const QJsonObject& structureRoot,
    const CommandTabMetadataCache& metadata
)
{
    struct PanelVisibilityCandidate {
        CommandTabPanelVisibilityEntry entry;
        int priority = 0;
        int sourceOrder = 0;
        bool fromGlobal = false;
        QString normalizedTitle;
        QString commandSignature;
        QSet<QString> commandSet;
    };

    auto signatureFromCommandSet = [](const QSet<QString>& commandSet) {
        QStringList ids = commandSet.values();
        std::sort(ids.begin(), ids.end());
        return ids.join(QStringLiteral("|"));
    };

    auto overlapCoverage = [](const QSet<QString>& first, const QSet<QString>& second) -> double {
        if (first.isEmpty() || second.isEmpty()) {
            return 0.0;
        }
        int intersection = 0;
        for (const auto& item : first) {
            if (second.contains(item)) {
                ++intersection;
            }
        }
        const qsizetype minimumSize = std::min(first.size(), second.size());
        if (minimumSize <= 0) {
            return 0.0;
        }
        return static_cast<double>(intersection) / static_cast<double>(minimumSize);
    };

    auto buildCommandSetFromToolbarPanel = [](const QJsonObject& panelObject) {
        QSet<QString> commandIds;
        for (const auto& commandId : orderedCommandIds(panelObject)) {
            const QString canonicalId = canonicalCommandIdFromToken(commandId);
            if (!canonicalId.isEmpty() && !isSeparatorCommand(canonicalId)) {
                commandIds.insert(canonicalId.toLower());
            }
        }
        return commandIds;
    };

    auto buildCommandSetFromCustomPanel = [](const QJsonObject& panelObject) {
        QSet<QString> commandIds;
        const auto commandsObject = panelObject.value(QStringLiteral("commands")).toObject();
        for (auto it = commandsObject.begin(); it != commandsObject.end(); ++it) {
            const QString canonicalId = canonicalCommandIdFromToken(it.key());
            if (!canonicalId.isEmpty() && !isSeparatorCommand(canonicalId)) {
                commandIds.insert(canonicalId.toLower());
            }
        }
        return commandIds;
    };

    auto buildCommandSetFromNewPanel = [](const QJsonArray& commandsArray) {
        QSet<QString> commandIds;
        for (const auto& commandValue : commandsArray) {
            if (!commandValue.isArray()) {
                continue;
            }
            const auto commandArray = commandValue.toArray();
            if (commandArray.isEmpty()) {
                continue;
            }
            const QString canonicalId = canonicalCommandIdFromToken(commandArray.at(0).toString());
            if (!canonicalId.isEmpty() && !isSeparatorCommand(canonicalId)) {
                commandIds.insert(canonicalId.toLower());
            }
        }
        return commandIds;
    };

    auto isBetterCandidate = [](const PanelVisibilityCandidate& candidate, const PanelVisibilityCandidate& existing) {
        if (candidate.priority != existing.priority) {
            return candidate.priority > existing.priority;
        }
        if (candidate.commandSet.size() != existing.commandSet.size()) {
            return candidate.commandSet.size() > existing.commandSet.size();
        }
        return candidate.sourceOrder < existing.sourceOrder;
    };

    QVector<PanelVisibilityCandidate> candidates;
    candidates.reserve(96);
    int sourceOrder = 0;

    auto appendCandidate = [&](CommandTabPanelVisibilityEntry entry, int priority, bool fromGlobal, const QSet<QString>& commandSet) {
        PanelVisibilityCandidate candidate;
        candidate.entry = std::move(entry);
        candidate.priority = priority;
        candidate.sourceOrder = sourceOrder++;
        candidate.fromGlobal = fromGlobal;
        candidate.commandSet = commandSet;
        candidate.commandSignature = signatureFromCommandSet(commandSet);
        candidate.normalizedTitle = normalizedPanelTitleKey(candidate.entry.title);
        candidates.push_back(candidate);
    };

    const auto newPanelsRoot = structureRoot.value(QStringLiteral("newPanels")).toObject();
    const auto workbenchesObject = structureRoot.value(QStringLiteral("workbenches")).toObject();
    for (auto it = workbenchesObject.begin(); it != workbenchesObject.end(); ++it) {
        if (!it.value().isObject()) {
            continue;
        }
        const QString workbenchId = it.key();
        const QString workbenchTitle = metadata.workbenchTitles.value(workbenchId, workbenchId);
        const auto toolbarsObject = it.value().toObject().value(QStringLiteral("toolbars")).toObject();
        QSet<QString> explicitOrderedToolbarIds;
        for (const auto& value : toolbarsObject.value(QStringLiteral("order")).toArray()) {
            const QString panelId = value.toString().trimmed();
            if (!panelId.isEmpty()) {
                explicitOrderedToolbarIds.insert(panelId);
            }
        }
        QSet<QString> newPanelIdsForWorkbench;
        for (const QString& source : {workbenchId, QStringLiteral("Global")}) {
            const auto panelsObject = newPanelsRoot.value(source).toObject();
            for (auto panelIt = panelsObject.begin(); panelIt != panelsObject.end(); ++panelIt) {
                const QString panelId = panelIt.key().trimmed();
                if (!panelId.isEmpty()) {
                    newPanelIdsForWorkbench.insert(panelId);
                }
            }
        }
        for (const auto& panelId : orderedToolbarIds(toolbarsObject)) {
            const bool isOrphanToolbar = !explicitOrderedToolbarIds.contains(panelId);
            if (isOrphanToolbar && newPanelIdsForWorkbench.contains(panelId)) {
                continue;
            }
            const auto panelObject = toolbarsObject.value(panelId).toObject();
            CommandTabPanelVisibilityEntry entry;
            entry.workbenchId = workbenchId;
            entry.workbenchTitle = workbenchTitle;
            entry.id = panelId;
            entry.title = metadata.panelTitles.value(panelId, panelObject.value(QStringLiteral("title")).toString(panelId));
            entry.visible = !isIgnoredToolbar(structureRoot, workbenchId, panelId, panelObject)
                && panelObject.value(QStringLiteral("Enabled")).toBool(true);
            appendCandidate(
                std::move(entry),
                isOrphanToolbar ? 300 : 400,
                false,
                buildCommandSetFromToolbarPanel(panelObject)
            );
        }

        const auto customPanelsObject = structureRoot.value(QStringLiteral("customToolbars")).toObject().value(workbenchId).toObject();
        for (auto panelIt = customPanelsObject.begin(); panelIt != customPanelsObject.end(); ++panelIt) {
            CommandTabPanelVisibilityEntry entry;
            entry.workbenchId = workbenchId;
            entry.workbenchTitle = workbenchTitle;
            entry.id = panelIt.key();
            entry.title = panelTitleFromId(panelIt.key(), QStringLiteral("_custom"));
            entry.visible = !isIgnoredToolbar(structureRoot, workbenchId, panelIt.key(), panelIt.value().toObject());
            appendCandidate(
                std::move(entry),
                200,
                false,
                buildCommandSetFromCustomPanel(panelIt.value().toObject())
            );
        }
    }

    for (auto sourceIt = newPanelsRoot.begin(); sourceIt != newPanelsRoot.end(); ++sourceIt) {
        if (!sourceIt.value().isObject()) {
            continue;
        }
        const QString workbenchId = sourceIt.key();
        const QString workbenchTitle = workbenchId == QStringLiteral("Global")
            ? QCoreApplication::translate("CommandTabCustomizationDialog", "Global")
            : metadata.workbenchTitles.value(workbenchId, workbenchId);
        const auto panelsObject = sourceIt.value().toObject();
        for (auto panelIt = panelsObject.begin(); panelIt != panelsObject.end(); ++panelIt) {
            CommandTabPanelVisibilityEntry entry;
            entry.workbenchId = workbenchId;
            entry.workbenchTitle = workbenchTitle;
            entry.id = panelIt.key();
            entry.title = panelTitleFromId(panelIt.key(), QStringLiteral("_newPanel"));
            entry.visible = !isIgnoredToolbar(structureRoot, workbenchId, panelIt.key(), QJsonObject());
            appendCandidate(
                std::move(entry),
                workbenchId == QStringLiteral("Global") ? 100 : 200,
                workbenchId == QStringLiteral("Global"),
                buildCommandSetFromNewPanel(panelIt.value().toArray())
            );
        }
    }

    QVector<PanelVisibilityCandidate> deduplicated;
    deduplicated.reserve(candidates.size());
    for (const auto& candidate : candidates) {
        bool handled = false;
        for (auto& existing : deduplicated) {
            if (candidate.entry.workbenchId != existing.entry.workbenchId) {
                continue;
            }
            const bool exactDuplicate =
                !candidate.commandSignature.isEmpty()
                && candidate.commandSignature == existing.commandSignature
                && candidate.normalizedTitle == existing.normalizedTitle;
            const bool commandSignatureDuplicate =
                !candidate.commandSignature.isEmpty()
                && candidate.commandSignature == existing.commandSignature
                && std::min(candidate.commandSet.size(), existing.commandSet.size()) >= 3;
            const double coverage = overlapCoverage(candidate.commandSet, existing.commandSet);
            const qsizetype minimumSize = std::min(candidate.commandSet.size(), existing.commandSet.size());
            const bool strongCollision =
                minimumSize >= 3
                && coverage >= 0.85
                && (
                    candidate.normalizedTitle == existing.normalizedTitle
                    || candidate.fromGlobal
                    || existing.fromGlobal
                );
            if (!exactDuplicate && !commandSignatureDuplicate && !strongCollision) {
                continue;
            }

            handled = true;
            if (isBetterCandidate(candidate, existing)) {
                existing = candidate;
            }
            break;
        }
        if (!handled) {
            deduplicated.push_back(candidate);
        }
    }

    QVector<CommandTabPanelVisibilityEntry> entries;
    entries.reserve(deduplicated.size());
    for (const auto& candidate : deduplicated) {
        entries.push_back(candidate.entry);
    }
    return entries;
}

QVector<CommandTabDropdownDefinitionEntry> buildDropdownDefinitionsFromStructure(
    const QJsonObject& structureRoot,
    const CommandTabMetadataCache& metadata,
    const CommandTabSettingsState& settings,
    const QString& addonRootPath
)
{
    QVector<CommandTabDropdownDefinitionEntry> entries;
    const auto dropdownsObject = structureRoot.value(QStringLiteral("dropdownButtons")).toObject();
    for (auto it = dropdownsObject.begin(); it != dropdownsObject.end(); ++it) {
        CommandTabDropdownDefinitionEntry entry;
        entry.id = it.key();
        entry.text = panelTitleFromId(it.key(), QStringLiteral("_ddb"));
        const auto commandsArray = it.value().toArray();
        for (const auto& commandValue : commandsArray) {
            if (!commandValue.isArray()) {
                continue;
            }
            const auto commandArray = commandValue.toArray();
            if (commandArray.isEmpty()) {
                continue;
            }
            const QString commandId = commandArray.at(0).toString().trimmed();
            if (commandId.isEmpty()) {
                continue;
            }
            const QString commandSourceWorkbenchId = commandArray.size() > 1
                ? commandArray.at(1).toString().trimmed()
                : QString();
            entry.commands.push_back(
                buildStructureCommand(
                    structureRoot,
                    commandId,
                    QJsonObject(),
                    metadata,
                    settings,
                    addonRootPath,
                    commandSourceWorkbenchId,
                    QString()
                )
            );
        }
        entries.push_back(entry);
    }
    return entries;
}

bool buildCommandTabModelFromBootstrap(const QJsonObject& root, CommandTabModel* model)
{
    if (model == nullptr) {
        setLastError(QStringLiteral("CommandTab model output is null"));
        return false;
    }

    const QString structurePath = root.value(QStringLiteral("structurePath")).toString();
    if (structurePath.isEmpty()) {
        setLastError(QStringLiteral("Native commandtab bootstrap is missing structurePath"));
        return false;
    }

    QJsonObject structureRoot;
    if (!loadJsonObjectFromFile(structurePath, &structureRoot, QStringLiteral("native commandtab structure"))) {
        return false;
    }

    CommandTabMetadataCache metadata;
    const QString metadataCachePath = root.value(QStringLiteral("metadataCachePath")).toString();
    if (!parseMetadataCacheFromPath(metadataCachePath, &metadata)) {
        return false;
    }
    const QString addonRootPath = addonRootFromStructurePath(structurePath);

    CommandTabModel nextModel;
    nextModel.activeWorkbenchId = root.value(QStringLiteral("activeWorkbenchId")).toString();
    if (root.value(QStringLiteral("settings")).isObject()) {
        nextModel.settings = parseSettingsState(root.value(QStringLiteral("settings")).toObject());
    }
    if (root.value(QStringLiteral("themeConfig")).isObject()) {
        nextModel.theme = resolveThemeFromConfig(parseThemeConfig(root.value(QStringLiteral("themeConfig")).toObject()));
    } else if (root.value(QStringLiteral("theme")).isObject()) {
        nextModel.theme = parseTheme(root.value(QStringLiteral("theme")).toObject());
    } else {
        nextModel.theme = resolveThemeFromConfig(NativeThemeConfig());
    }
    nextModel.workbenchVisibility = buildWorkbenchVisibilityFromStructure(
        structureRoot,
        metadata,
        addonRootPath
    );
    nextModel.panelVisibility = buildPanelVisibilityFromStructure(structureRoot, metadata);
    nextModel.dropdownDefinitions = buildDropdownDefinitionsFromStructure(
        structureRoot,
        metadata,
        nextModel.settings,
        addonRootPath
    );

    const auto quickAccessArray = structureRoot.value(QStringLiteral("quickAccessCommands")).toArray();
    for (const auto& value : quickAccessArray) {
        const QString commandId = value.toString();
        if (commandId.isEmpty()) {
            continue;
        }
        nextModel.quickAccess.push_back(
            buildStructureCommand(
                structureRoot,
                commandId,
                QJsonObject(),
                metadata,
                nextModel.settings,
                addonRootPath
            )
        );
    }

    const bool includeAllPanels = root.value(QStringLiteral("includeAllPanels")).toBool(false);
    const auto workbenchesObject = structureRoot.value(QStringLiteral("workbenches")).toObject();
    for (auto it = workbenchesObject.begin(); it != workbenchesObject.end(); ++it) {
        if (!it.value().isObject()) {
            continue;
        }
        const QString workbenchId = it.key();
        const QString normalizedWorkbenchId = workbenchId.trimmed().toLower();
        if (
            normalizedWorkbenchId == QStringLiteral("noneworkbench")
            || normalizedWorkbenchId == QStringLiteral("<none>")
            || normalizedWorkbenchId == QStringLiteral("none")
        ) {
            continue;
        }
        const auto workbenchObject = it.value().toObject();
        if (!workbenchHasEnabledPanels(structureRoot, workbenchId, workbenchObject)) {
            continue;
        }

        CommandTabWorkbenchEntry workbench;
        workbench.id = workbenchId;
        workbench.title = cleanedWorkbenchTitle(metadata.workbenchTitles.value(workbenchId, workbenchId), workbenchId);
        const QString normalizedWorkbenchTitle = workbench.title.trimmed().toLower();
        if (
            normalizedWorkbenchTitle == QStringLiteral("<none>")
            || normalizedWorkbenchTitle == QStringLiteral("none")
        ) {
            continue;
        }
        workbench.iconPath = metadata.workbenchIcons.value(workbenchId);
        if (workbench.iconPath.isEmpty()) {
            workbench.iconPath = resolveWorkbenchIconSource(addonRootPath, workbenchId, workbench.title);
        }
        if (includeAllPanels || workbenchId == nextModel.activeWorkbenchId) {
            workbench = buildWorkbenchFromStructure(
                structureRoot,
                workbenchId,
                workbenchObject,
                metadata,
                nextModel.settings,
                addonRootPath
            );
        }
        nextModel.workbenches.push_back(workbench);
    }

    *model = nextModel;
    return true;
}

bool buildWorkbenchFromBootstrap(const QJsonObject& root, CommandTabWorkbenchEntry* workbench)
{
    if (workbench == nullptr) {
        setLastError(QStringLiteral("Workbench output is null"));
        return false;
    }

    const QString structurePath = root.value(QStringLiteral("structurePath")).toString();
    const QString workbenchId = root.value(QStringLiteral("workbenchId")).toString();
    if (structurePath.isEmpty() || workbenchId.isEmpty()) {
        setLastError(QStringLiteral("Native commandtab workbench bootstrap is incomplete"));
        return false;
    }
    {
        const QString normalizedWorkbenchId = workbenchId.trimmed().toLower();
        if (
            normalizedWorkbenchId == QStringLiteral("noneworkbench")
            || normalizedWorkbenchId == QStringLiteral("<none>")
            || normalizedWorkbenchId == QStringLiteral("none")
        ) {
            setLastError(QStringLiteral("Placeholder workbench is not allowed in native commandtab"));
            return false;
        }
    }

    QJsonObject structureRoot;
    if (!loadJsonObjectFromFile(structurePath, &structureRoot, QStringLiteral("native commandtab structure"))) {
        return false;
    }

    CommandTabMetadataCache metadata;
    if (!parseMetadataCacheFromPath(root.value(QStringLiteral("metadataCachePath")).toString(), &metadata)) {
        return false;
    }
    const QString addonRootPath = addonRootFromStructurePath(structurePath);
    CommandTabSettingsState settings;
    if (root.value(QStringLiteral("settings")).isObject()) {
        settings = parseSettingsState(root.value(QStringLiteral("settings")).toObject());
    }

    const auto workbenchesObject = structureRoot.value(QStringLiteral("workbenches")).toObject();
    const auto workbenchValue = workbenchesObject.value(workbenchId);
    if (!workbenchValue.isObject()) {
        setLastError(QStringLiteral("Workbench not found in native commandtab structure: %1").arg(workbenchId));
        return false;
    }

    *workbench = buildWorkbenchFromStructure(
        structureRoot,
        workbenchId,
        workbenchValue.toObject(),
        metadata,
        settings,
        addonRootPath
    );
    return true;
}

bool parseCommandTabModelDocument(const QJsonDocument& document, CommandTabModel* model)
{
    if (model == nullptr) {
        setLastError(QStringLiteral("CommandTab model output is null"));
        return false;
    }

    if (!document.isObject()) {
        setLastError(QStringLiteral("Invalid native commandtab model JSON: root object expected"));
        return false;
    }

    const auto root = document.object();
    if (root.value(QStringLiteral("structurePath")).isString()) {
        return buildCommandTabModelFromBootstrap(root, model);
    }

    CommandTabModel nextModel;
    nextModel.activeWorkbenchId = root.value(QStringLiteral("activeWorkbenchId")).toString();
    if (root.value(QStringLiteral("settings")).isObject()) {
        nextModel.settings = parseSettingsState(root.value(QStringLiteral("settings")).toObject());
    }
    if (root.value(QStringLiteral("themeConfig")).isObject()) {
        nextModel.theme = resolveThemeFromConfig(parseThemeConfig(root.value(QStringLiteral("themeConfig")).toObject()));
    } else if (root.value(QStringLiteral("theme")).isObject()) {
        nextModel.theme = parseTheme(root.value(QStringLiteral("theme")).toObject());
    } else {
        nextModel.theme = resolveThemeFromConfig(NativeThemeConfig());
    }

    const auto quickAccess = root.value(QStringLiteral("quickAccess")).toArray();
    for (const auto& commandValue : quickAccess) {
        if (!commandValue.isObject()) {
            continue;
        }
        nextModel.quickAccess.push_back(parseCommand(commandValue.toObject()));
    }

    const auto workbenchVisibility = root.value(QStringLiteral("workbenchVisibility")).toArray();
    for (const auto& workbenchValue : workbenchVisibility) {
        if (!workbenchValue.isObject()) {
            continue;
        }
        nextModel.workbenchVisibility.push_back(parseWorkbenchVisibility(workbenchValue.toObject()));
    }

    const auto panelVisibility = root.value(QStringLiteral("panelVisibility")).toArray();
    for (const auto& panelValue : panelVisibility) {
        if (!panelValue.isObject()) {
            continue;
        }
        nextModel.panelVisibility.push_back(parsePanelVisibility(panelValue.toObject()));
    }

    const auto dropdownDefinitions = root.value(QStringLiteral("dropdownDefinitions")).toArray();
    for (const auto& dropdownValue : dropdownDefinitions) {
        if (!dropdownValue.isObject()) {
            continue;
        }
        nextModel.dropdownDefinitions.push_back(parseDropdownDefinition(dropdownValue.toObject()));
    }

    const auto workbenches = root.value(QStringLiteral("workbenches")).toArray();
    for (const auto& workbenchValue : workbenches) {
        if (!workbenchValue.isObject()) {
            continue;
        }
        CommandTabWorkbenchEntry workbench = parseWorkbench(workbenchValue.toObject());

        if (!workbench.id.isEmpty()) {
            nextModel.workbenches.push_back(workbench);
        }
    }

    *model = nextModel;
    return true;
}

bool parseCommandTabModelJson(const QByteArray& payload, CommandTabModel* model)
{
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        setLastError(QStringLiteral("Invalid native commandtab model JSON: %1").arg(parseError.errorString()));
        return false;
    }
    return parseCommandTabModelDocument(document, model);
}

bool loadCommandTabModelFromJson(const QString& path, CommandTabModel* model)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setLastError(QStringLiteral("Unable to open native commandtab model: %1").arg(path));
        return false;
    }

    return parseCommandTabModelJson(file.readAll(), model);
}

bool parseWorkbenchJson(const QByteArray& payload, CommandTabWorkbenchEntry* workbench)
{
    if (workbench == nullptr) {
        setLastError(QStringLiteral("Workbench output is null"));
        return false;
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setLastError(QStringLiteral("Invalid native commandtab workbench JSON: %1").arg(parseError.errorString()));
        return false;
    }

    const auto root = document.object();
    CommandTabWorkbenchEntry parsedWorkbench;
    if (root.value(QStringLiteral("structurePath")).isString()) {
        if (!buildWorkbenchFromBootstrap(root, &parsedWorkbench)) {
            return false;
        }
    } else {
        parsedWorkbench = parseWorkbench(root);
    }
    if (parsedWorkbench.id.isEmpty()) {
        setLastError(QStringLiteral("Invalid native commandtab workbench JSON: missing id"));
        return false;
    }

    *workbench = parsedWorkbench;
    return true;
}

void clearLayout(QLayout* layout)
{
    if (layout == nullptr) {
        return;
    }

    while (auto* item = layout->takeAt(0)) {
        if (auto* childLayout = item->layout()) {
            clearLayout(childLayout);
            delete childLayout;
        }
        if (auto* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

[[maybe_unused]] QString wrapCommandTabLargeText(const QString& inputText, int preferredLineLength = 14)
{
    QString text = inputText.simplified();
    if (text.isEmpty() || text.contains(QLatin1Char('\n')) || text.size() <= preferredLineLength) {
        return text;
    }

    const QStringList words = text.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (words.size() <= 1) {
        const qsizetype midpoint = text.size() / 2;
        return text.left(midpoint).trimmed() + QStringLiteral("\n") + text.mid(midpoint).trimmed();
    }

    QString firstLine;
    QString secondLine;
    for (const auto& word : words) {
        const QString candidate = firstLine.isEmpty() ? word : firstLine + QStringLiteral(" ") + word;
        if (firstLine.isEmpty() || candidate.size() <= preferredLineLength || secondLine.isEmpty()) {
            if (candidate.size() <= preferredLineLength || firstLine.isEmpty()) {
                firstLine = candidate;
                continue;
            }
        }

        secondLine = secondLine.isEmpty() ? word : secondLine + QStringLiteral(" ") + word;
    }

    if (secondLine.isEmpty()) {
        const qsizetype splitIndex = firstLine.lastIndexOf(QLatin1Char(' '));
        if (splitIndex > 0) {
            secondLine = firstLine.mid(splitIndex + 1);
            firstLine = firstLine.left(splitIndex);
        }
    }

    if (secondLine.isEmpty()) {
        return firstLine;
    }
    return firstLine.trimmed() + QStringLiteral("\n") + secondLine.trimmed();
}

QStringList wrapCommandTabSmallText(
    const QString& inputText,
    const QFontMetrics& metrics,
    int maxWidth,
    int maxLines = 3
)
{
    const QString simplified = inputText.simplified();
    if (simplified.isEmpty()) {
        return {};
    }

    const QStringList words = simplified.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    QStringList lines;
    QString currentLine;
    int wordIndex = 0;
    for (; wordIndex < words.size(); ++wordIndex) {
        const auto& word = words.at(wordIndex);
        const QString candidate =
            currentLine.isEmpty() ? word : currentLine + QStringLiteral(" ") + word;
        if (metrics.horizontalAdvance(candidate) <= maxWidth) {
            currentLine = candidate;
            continue;
        }

        if (!currentLine.isEmpty()) {
            lines.push_back(currentLine);
        }
        currentLine = word;
        if (lines.size() >= maxLines - 1) {
            ++wordIndex;
            break;
        }
    }

    QString remainingText = currentLine;
    if (wordIndex < words.size()) {
        const QStringList remainingWords = words.mid(wordIndex);
        if (!remainingText.isEmpty() && !remainingWords.isEmpty()) {
            remainingText += QStringLiteral(" ");
        }
        remainingText += remainingWords.join(QStringLiteral(" "));
    }

    if (!remainingText.isEmpty()) {
        lines.push_back(metrics.elidedText(remainingText, Qt::ElideRight, maxWidth));
    }

    if (lines.isEmpty()) {
        lines.push_back(metrics.elidedText(simplified, Qt::ElideRight, maxWidth));
    }
    return lines.mid(0, maxLines);
}

QStringList wrapCommandTabTextExact(
    const QString& inputText,
    const QFontMetrics& metrics,
    int maxWidth
)
{
    const QString simplified = inputText.simplified();
    if (simplified.isEmpty()) {
        return {};
    }

    const QStringList words = simplified.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    QStringList lines;
    QString currentLine;
    for (const auto& word : words) {
        const QString candidate =
            currentLine.isEmpty() ? word : currentLine + QStringLiteral(" ") + word;
        if (currentLine.isEmpty() || metrics.horizontalAdvance(candidate) <= maxWidth) {
            currentLine = candidate;
            continue;
        }

        lines.push_back(currentLine);
        currentLine = word;
    }

    if (!currentLine.isEmpty()) {
        lines.push_back(currentLine);
    }
    return lines;
}

QStringList wrapCommandTabTextToWidth(
    const QString& inputText,
    const QFontMetrics& metrics,
    int maxWidth,
    int maxLines
)
{
    const int safeWidth = std::max(32, maxWidth);
    const QStringList exactLines = wrapCommandTabTextExact(inputText, metrics, safeWidth);
    if (exactLines.size() <= maxLines) {
        return exactLines;
    }
    return wrapCommandTabSmallText(inputText, metrics, safeWidth, maxLines);
}

[[maybe_unused]] int normalizeCommandTabColumnWidth(int width)
{
    const int clampedWidth = std::clamp(width, 120, 600);
    const int remainder = clampedWidth % 6;
    if (remainder == 0) {
        return clampedWidth;
    }
    const int rounded = clampedWidth + (6 - remainder);
    return std::clamp(rounded, 120, 600);
}

bool commandtabLinesElided(const QStringList& lines)
{
    for (const auto& line : lines) {
        if (line.contains(QChar(0x2026)) || line.contains(QStringLiteral("..."))) {
            return true;
        }
    }
    return false;
}

int commandtabButtonLineHeight(const QFontMetrics& metrics)
{
    return std::max(metrics.height(), metrics.lineSpacing()) + 1;
}

int commandtabReservedTextHeight(const QFontMetrics& metrics)
{
    return commandtabButtonLineHeight(metrics) * 2 + 2;
}

int commandtabPanelFooterHeight(const QFontMetrics& metrics)
{
    // Keep enough vertical space so rounded title bars remain visually clean.
    return std::max(26, commandtabButtonLineHeight(metrics) + 10);
}

enum class CommandTabButtonSizeKind {
    Small,
    Medium,
    Large
};

CommandTabButtonSizeKind parseCommandTabButtonSizeKind(const QString& size)
{
    if (size.compare(QStringLiteral("large"), Qt::CaseInsensitive) == 0) {
        return CommandTabButtonSizeKind::Large;
    }
    if (size.compare(QStringLiteral("medium"), Qt::CaseInsensitive) == 0) {
        return CommandTabButtonSizeKind::Medium;
    }
    return CommandTabButtonSizeKind::Small;
}
