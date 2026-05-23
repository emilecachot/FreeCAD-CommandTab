class SplitScrollTabBarStyle final : public QProxyStyle
{
public:
    explicit SplitScrollTabBarStyle(QStyle* baseStyle = nullptr, int buttonWidth = 32)
        : QProxyStyle(baseStyle)
        , m_buttonWidth(std::max(16, buttonWidth))
    {
    }

    int pixelMetric(
        PixelMetric metric,
        const QStyleOption* option = nullptr,
        const QWidget* widget = nullptr
    ) const override
    {
        if (metric == QStyle::PM_TabBarScrollButtonWidth) {
            return m_buttonWidth;
        }
        return QProxyStyle::pixelMetric(metric, option, widget);
    }

    QRect subElementRect(
        SubElement element,
        const QStyleOption* option,
        const QWidget* widget = nullptr
    ) const override
    {
        const auto* tabBar = qobject_cast<const QTabBar*>(widget);
        if (tabBar != nullptr) {
            const int bw = m_buttonWidth;
            const int h = std::max(1, tabBar->height());
            const int w = std::max(0, tabBar->width());
            if (element == QStyle::SE_TabBarScrollLeftButton) {
                return QRect(0, 0, bw, h);
            }
            if (element == QStyle::SE_TabBarScrollRightButton) {
                return QRect(std::max(0, w - bw), 0, bw, h);
            }
        }
        return QProxyStyle::subElementRect(element, option, widget);
    }

private:
    int m_buttonWidth = 32;
};

class CommandTabShellWidget final : public QWidget
{
public:
    explicit CommandTabShellWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto trLabel = [](const char* text) {
            return QCoreApplication::translate("CommandTabShellWidget", text);
        };

        setObjectName(QStringLiteral("FreeCADCommandTabNativeShell"));
        setAttribute(Qt::WA_StyledBackground, true);
        applyThemeStyleSheet();

        auto* outerLayout = new QVBoxLayout(this);
        outerLayout->setContentsMargins(8, 6, 8, 6);
        outerLayout->setSpacing(4);

        m_topBarWidget = new QWidget(this);
        m_topBarWidget->setObjectName(QStringLiteral("CommandTabHeader"));
        m_topBarWidget->setAttribute(Qt::WA_StyledBackground, true);
        m_topBarWidget->setMinimumHeight(46);
        installRoundedMask(m_topBarWidget, scaledPx(10));
        m_topBarLayout = new QHBoxLayout(m_topBarWidget);
        m_topBarLayout->setContentsMargins(10, 5, 10, 5);
        m_topBarLayout->setSpacing(8);
        outerLayout->addWidget(m_topBarWidget);

        m_quickAccessWidget = new QWidget(m_topBarWidget);
        m_quickAccessWidget->setObjectName(QStringLiteral("CommandTabQuickAccessBar"));
        m_quickAccessWidget->setAttribute(Qt::WA_StyledBackground, true);
        installRoundedMask(m_quickAccessWidget, scaledPx(8));
        m_quickAccessLayout = new QHBoxLayout(m_quickAccessWidget);
        m_quickAccessLayout->setContentsMargins(5, 4, 5, 4);
        m_quickAccessLayout->setSpacing(4);
        m_topBarLayout->addWidget(m_quickAccessWidget, 0, Qt::AlignVCenter | Qt::AlignLeft);

        m_brandWidget = new QLabel(m_topBarWidget);
        m_brandWidget->setObjectName(QStringLiteral("CommandTabBrandBadge"));
        m_brandWidget->setFrameStyle(QFrame::NoFrame);
        installRoundedMask(m_brandWidget, scaledPx(6));
        m_brandWidget->setAlignment(Qt::AlignCenter);
        m_brandWidget->setFixedSize(42, 32);
        m_brandWidget->setToolTip(trLabel("FreeCAD CommandTab"));
        m_topBarLayout->addWidget(m_brandWidget, 0, Qt::AlignVCenter);

        m_tabBar = new QTabBar(m_topBarWidget);
        m_tabBar->setObjectName(QStringLiteral("CommandTabTabBar"));
        m_tabBar->setAttribute(Qt::WA_StyledBackground, true);
        installRoundedMask(m_tabBar, scaledPx(10));
        m_tabBar->setExpanding(false);
        m_tabBar->setDocumentMode(true);
        m_tabBar->setDrawBase(false);
        m_tabBar->setUsesScrollButtons(true);
        m_tabBar->setElideMode(Qt::ElideNone);
        m_tabBar->setIconSize(workbenchTabIconSize());
        m_tabBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_tabBar->setStyle(new SplitScrollTabBarStyle(m_tabBar->style(), scaledHeaderPx(32)));
        QFont tabFont = m_tabBar->font();
        tabFont.setPointSize(10);
        tabFont.setBold(true);
        m_tabBar->setFont(tabFont);
        m_tabBar->setMouseTracking(true);
        m_tabBar->installEventFilter(this);
        m_topBarLayout->addWidget(m_tabBar, 1, Qt::AlignVCenter);

        m_gridToggleButton = new QToolButton(m_topBarWidget);
        m_gridToggleButton->setObjectName(QStringLiteral("CommandTabGridToggleButton"));
        m_gridToggleButton->setAutoRaise(true);
        m_gridToggleButton->setCheckable(false);
        m_gridToggleButton->setToolTip(
            QCoreApplication::translate("CommandTabShellWidget", "Toggle grid")
        );
        m_gridToggleButton->setVisible(false); // shown via rebuildHeaderCommandWidgets
        m_topBarLayout->addWidget(m_gridToggleButton, 0, Qt::AlignVCenter);
        connect(m_gridToggleButton, &QToolButton::clicked, this, [this]() {
            if (m_commandHandler) {
                m_commandHandler(QStringLiteral("__commandtab_toggle_grid__"));
            }
        });

        m_utilityWidget = new QWidget(m_topBarWidget);
        m_utilityWidget->setObjectName(QStringLiteral("CommandTabUtilityBar"));
        m_utilityWidget->setAttribute(Qt::WA_StyledBackground, true);
        installRoundedMask(m_utilityWidget, scaledPx(8));
        m_utilityLayout = new QHBoxLayout(m_utilityWidget);
        m_utilityLayout->setContentsMargins(5, 4, 5, 4);
        m_utilityLayout->setSpacing(6);
        m_topBarLayout->addWidget(m_utilityWidget, 0, Qt::AlignVCenter | Qt::AlignRight);

        m_stack = new QStackedWidget(this);
        outerLayout->addWidget(m_stack, 1);

        updateBrandWidget();
        applyShellDisplayScaleMetrics();

        connect(m_tabBar, &QTabBar::currentChanged, this, [this](int index) {
            applyTabTextColors();
            updateTabNavigationButtons();
            const bool targetHasPanels =
                index >= 0 &&
                index < m_workbenchEntries.size() &&
                !m_workbenchEntries.at(index).panels.isEmpty();
            if (targetHasPanels) {
                if (!m_settingsState.tabClickPopupMode) {
                    ensureWorkbenchPage(index);
                    m_stack->setCurrentIndex(index);
                } else if (!m_ribbonCollapsed) {
                    setRibbonCollapsed(true, true);
                }
            }
            if (
                m_ribbonCollapsed
                && m_settingsState.ribbonAutoHide
                && !m_settingsState.tabClickPopupMode
            ) {
                setRibbonCollapsed(false, true);
            }
            if (m_loadingModel || index < 0 || index >= m_workbenchIds.size()) {
                return;
            }
            if (m_commandHandler) {
                m_commandHandler(QStringLiteral("__workbench__:%1").arg(m_workbenchIds.at(index)));
            }
        });

        connect(m_tabBar, &QTabBar::tabBarClicked, this, [this](int index) {
            if (!m_settingsState.tabClickPopupMode) {
                return;
            }
            if (index < 0 || index >= m_workbenchEntries.size()) {
                return;
            }
            if (m_workbenchEntries.at(index).panels.isEmpty()) {
                return;
            }
            if (!m_ribbonCollapsed) {
                setRibbonCollapsed(true, true);
            }
            showWorkbenchTabPopup(index);
        });
        updateTabNavigationButtons();
    }

    QString translatedShellText(const char* text) const
    {
        const QString sourceText = QString::fromUtf8(text);
        const QString shellText = QCoreApplication::translate("CommandTabShellWidget", text);
        if (shellText.trimmed().isEmpty() || shellText == sourceText) {
            const QString globalText = QCoreApplication::translate("FreeCAD CommandTab", text);
            if (!globalText.trimmed().isEmpty() && globalText != sourceText) {
                return globalText;
            }
        }
        return shellText;
    }

    void setCommandHandler(std::function<void(const QString&)> handler)
    {
        m_commandHandler = std::move(handler);
    }

    void setContentChangedHandler(std::function<void()> handler)
    {
        m_contentChangedHandler = std::move(handler);
    }

    void openSettingsDialog(const CommandTabSettingsState& state)
    {
        if (m_settingsDialog == nullptr) {
            m_settingsDialog = new CommandTabSettingsDialog(this);
            m_settingsDialog->setAttribute(Qt::WA_DeleteOnClose, false);
            m_settingsDialog->setCommandHandler([this](const QString& command) {
                if (m_commandHandler) {
                    m_commandHandler(command);
                }
            });
        }

        m_settingsDialog->setTheme(m_theme);
        m_settingsDialog->setState(state);
        m_settingsDialog->show();
        m_settingsDialog->raise();
        m_settingsDialog->activateWindow();
    }

    QMainWindow* resolveMainWindow() const
    {
        if (auto* mainWindow = qobject_cast<QMainWindow*>(window())) {
            return mainWindow;
        }

        QWidget* current = parentWidget();
        while (current != nullptr) {
            if (auto* mainWindow = qobject_cast<QMainWindow*>(current)) {
                return mainWindow;
            }
            current = current->parentWidget();
        }
        return nullptr;
    }

    void invalidateActionLookup() const
    {
        m_actionsByCommandId.clear();
        m_missingActionCommandIds.clear();
        m_actionLookupReady = false;
    }

    void rebuildActionLookup() const
    {
        if (m_actionLookupReady) {
            return;
        }

        m_actionsByCommandId.clear();

        QMainWindow* mainWindow = resolveMainWindow();
        if (mainWindow == nullptr) {
            m_actionLookupReady = false;
            return;
        }

        const auto actions = collectUniqueWindowActions(mainWindow);
        for (auto* action : actions) {
            const QString commandId = actionCommandId(action);
            if (commandId.isEmpty()) {
                continue;
            }
            if (!m_actionsByCommandId.contains(commandId)) {
                m_actionsByCommandId.insert(commandId, QPointer<QAction>(action));
            }
        }
        m_actionLookupReady = true;
    }

    QAction* resolveActionForCommandId(const QString& commandId) const
    {
        if (!m_actionLookupReady) {
            rebuildActionLookup();
        }

        const QString normalizedId = normalizeActionCandidate(commandId);
        if (normalizedId.isEmpty()) {
            return nullptr;
        }
        if (m_missingActionCommandIds.contains(normalizedId)) {
            return nullptr;
        }

        const auto directIt = m_actionsByCommandId.constFind(normalizedId);
        if (directIt != m_actionsByCommandId.constEnd() && !directIt.value().isNull()) {
            m_missingActionCommandIds.remove(normalizedId);
            return directIt.value().data();
        }

        const qsizetype commaIndex = normalizedId.indexOf(QLatin1Char(','));
        if (commaIndex > 0) {
            const QString baseId = normalizeActionCandidate(normalizedId.left(commaIndex));
            if (!baseId.isEmpty()) {
                if (m_missingActionCommandIds.contains(baseId)) {
                    m_missingActionCommandIds.insert(normalizedId);
                    return nullptr;
                }
                const auto baseIt = m_actionsByCommandId.constFind(baseId);
                if (baseIt != m_actionsByCommandId.constEnd() && !baseIt.value().isNull()) {
                    m_missingActionCommandIds.remove(baseId);
                    m_missingActionCommandIds.remove(normalizedId);
                    return baseIt.value().data();
                }
            }
        }

        if (!m_actionLookupRetryInProgress) {
            m_actionLookupRetryInProgress = true;
            invalidateActionLookup();
            rebuildActionLookup();
            m_actionLookupRetryInProgress = false;

            const auto retryDirectIt = m_actionsByCommandId.constFind(normalizedId);
            if (retryDirectIt != m_actionsByCommandId.constEnd() && !retryDirectIt.value().isNull()) {
                m_missingActionCommandIds.remove(normalizedId);
                return retryDirectIt.value().data();
            }
            if (commaIndex > 0) {
                const QString baseId = normalizeActionCandidate(normalizedId.left(commaIndex));
                if (!baseId.isEmpty()) {
                    const auto retryBaseIt = m_actionsByCommandId.constFind(baseId);
                    if (retryBaseIt != m_actionsByCommandId.constEnd() && !retryBaseIt.value().isNull()) {
                        m_missingActionCommandIds.remove(baseId);
                        m_missingActionCommandIds.remove(normalizedId);
                        return retryBaseIt.value().data();
                    }
                }
            }
        }

        m_missingActionCommandIds.insert(normalizedId);
        if (commaIndex > 0) {
            const QString baseId = normalizeActionCandidate(normalizedId.left(commaIndex));
            if (!baseId.isEmpty()) {
                m_missingActionCommandIds.insert(baseId);
            }
        }

        return nullptr;
    }

    void bindWidgetEnabledToCommand(QWidget* widget, const QString& commandId)
    {
        if (widget == nullptr) {
            return;
        }

        QAction* sourceAction = resolveActionForCommandId(commandId);
        if (sourceAction == nullptr) {
            widget->setEnabled(true);
            return;
        }

        widget->setEnabled(sourceAction->isEnabled());
        QPointer<QWidget> guardedWidget(widget);
        QPointer<QAction> guardedAction(sourceAction);
        connect(sourceAction, &QAction::changed, widget, [guardedWidget, guardedAction]() {
            if (guardedWidget.isNull()) {
                return;
            }
            if (guardedAction.isNull()) {
                guardedWidget->setEnabled(true);
                return;
            }
            guardedWidget->setEnabled(guardedAction->isEnabled());
        });
    }

    void bindMenuActionEnabledToCommand(QAction* targetAction, const QString& commandId)
    {
        if (targetAction == nullptr) {
            return;
        }

        QAction* sourceAction = resolveActionForCommandId(commandId);
        if (sourceAction == nullptr) {
            targetAction->setEnabled(true);
            return;
        }

        targetAction->setEnabled(sourceAction->isEnabled());
        QPointer<QAction> guardedTarget(targetAction);
        QPointer<QAction> guardedSource(sourceAction);
        connect(sourceAction, &QAction::changed, targetAction, [guardedTarget, guardedSource]() {
            if (guardedTarget.isNull()) {
                return;
            }
            if (guardedSource.isNull()) {
                guardedTarget->setEnabled(true);
                return;
            }
            guardedTarget->setEnabled(guardedSource->isEnabled());
        });
    }

    void bindWidgetCheckedToCommand(CommandTabCommandButton* button, const QString& commandId)
    {
        if (button == nullptr) {
            return;
        }

        QAction* sourceAction = resolveActionForCommandId(commandId);
        if (sourceAction == nullptr) {
            return;
        }

        button->setChecked(sourceAction->isChecked());
        QPointer<CommandTabCommandButton> guardedButton(button);
        QPointer<QAction> guardedAction(sourceAction);
        connect(sourceAction, &QAction::changed, button, [guardedButton, guardedAction]() {
            if (guardedButton.isNull()) {
                return;
            }
            if (guardedAction.isNull()) {
                guardedButton->setChecked(false);
                return;
            }
            guardedButton->setChecked(guardedAction->isChecked());
        });
    }

    bool setModel(const CommandTabModel& model)
    {
        m_loadingModel = true;
        setUpdatesEnabled(false);
        clearLoadedIconCache();
        invalidateActionLookup();
        m_model = model;
        m_settingsState = model.settings;
        m_baseTheme = model.theme;
        m_theme = normalizeShellTheme(applySettingsThemeOverrides(m_baseTheme, m_settingsState));
        m_model.theme = m_theme;
        applyThemeStyleSheet();
        applyShellDisplayScaleMetrics();
        if (m_customizationDialog != nullptr) {
            m_customizationDialog->setTheme(m_theme);
            m_customizationDialog->setModel(m_model);
        }
        m_workbenchIds.clear();
        m_workbenchEntries.clear();
        m_workbenchPagesBuilt.clear();
        m_workbenchPagesDirty.clear();
        m_pageBuildStates.clear();
        m_recentPanelCommandHistory.clear();
        m_pendingPanelPrimaryRefresh.clear();

        rebuildHeaderCommandWidgets();

        while (m_stack->count() > 0) {
            auto* page = m_stack->widget(0);
            m_stack->removeWidget(page);
            page->deleteLater();
        }
        while (m_tabBar->count() > 0) {
            m_tabBar->removeTab(0);
        }

        QSignalBlocker tabBlocker(m_tabBar);
        int activeIndex = 0;
        for (int modelIndex = 0; modelIndex < model.workbenches.size(); ++modelIndex) {
            const auto& workbench = model.workbenches.at(modelIndex);
            const QString normalizedWorkbenchId = workbench.id.trimmed().toLower();
            const QString normalizedWorkbenchTitle = workbench.title.trimmed().toLower();
            if (
                normalizedWorkbenchId == QStringLiteral("noneworkbench")
                || normalizedWorkbenchId == QStringLiteral("<none>")
                || normalizedWorkbenchId == QStringLiteral("none")
                || normalizedWorkbenchTitle == QStringLiteral("<none>")
                || normalizedWorkbenchTitle == QStringLiteral("none")
            ) {
                continue;
            }
            m_workbenchEntries.push_back(workbench);
            m_workbenchIds.push_back(workbench.id);
            m_workbenchPagesBuilt.push_back(false);
            m_workbenchPagesDirty.push_back(false);
            m_pageBuildStates.push_back(PageBuildState());
            const QIcon workbenchIcon = resolveWorkbenchIcon(workbench);
            m_tabBar->addTab(workbenchIcon, workbench.title);
            const int tabIndex = static_cast<int>(m_workbenchEntries.size() - 1);
            m_tabBar->setTabToolTip(tabIndex, workbench.title);
            auto* placeholder = new QWidget(m_stack);
            placeholder->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
            m_stack->addWidget(placeholder);
            if (!model.activeWorkbenchId.isEmpty() && workbench.id == model.activeWorkbenchId) {
                activeIndex = tabIndex;
            }
        }

        if (m_tabBar->count() > 0) {
            ensureWorkbenchPage(activeIndex, 4, true);
            m_tabBar->setCurrentIndex(activeIndex);
            m_stack->setCurrentIndex(activeIndex);
            scheduleBackgroundPageWarmup();
        }
        applyTabTextColors();
        updateTabNavigationButtons();
        scheduleWorkbenchTabIconRefresh(120);
        scheduleCommandIconRefresh(40, 2);

        setUpdatesEnabled(true);
        updateGeometry();
        m_loadingModel = false;
        return true;
    }

    bool setActiveWorkbench(const QString& workbenchId)
    {
        const qsizetype indexValue = m_workbenchIds.indexOf(workbenchId);
        if (indexValue < 0) {
            return false;
        }
        const int index = static_cast<int>(indexValue);

        ensureWorkbenchPage(index);
        QSignalBlocker tabBlocker(m_tabBar);
        m_tabBar->setCurrentIndex(index);
        m_stack->setCurrentIndex(index);
        m_model.activeWorkbenchId = workbenchId;
        updateTabNavigationButtons();
        if (m_customizationDialog != nullptr) {
            m_customizationDialog->setModel(m_model);
        }
        scheduleBackgroundPageWarmup();
        scheduleWorkbenchTabIconRefresh(80);
        scheduleCommandIconRefresh(30, 2);
        return true;
    }

    bool applySettings(const CommandTabSettingsState& state)
    {
        const bool themeOverrideChanged =
            state.customMainColorsEnabled != m_settingsState.customMainColorsEnabled
            || state.customMainBackgroundColor != m_settingsState.customMainBackgroundColor
            || state.customMainTextColor != m_settingsState.customMainTextColor
            || state.customRibbonPrimaryColor != m_settingsState.customRibbonPrimaryColor
            || state.customRibbonSecondaryColor != m_settingsState.customRibbonSecondaryColor
            || state.customRibbonAccentColor != m_settingsState.customRibbonAccentColor;
        const bool textVisibilityChanged =
            state.showIconTextSmall != m_settingsState.showIconTextSmall
            || state.showIconTextMedium != m_settingsState.showIconTextMedium
            || state.showIconTextLarge != m_settingsState.showIconTextLarge;
        const bool iconOnlySizeChanged =
            state.iconOnlySizeSmall != m_settingsState.iconOnlySizeSmall
            || state.iconOnlySizeMedium != m_settingsState.iconOnlySizeMedium
            || state.iconOnlySizeLarge != m_settingsState.iconOnlySizeLarge;
        const bool compactLayoutChanged =
            state.compactPanelLayout != m_settingsState.compactPanelLayout
            || state.compactPanelSpacing != m_settingsState.compactPanelSpacing
            || state.compactButtonPadding != m_settingsState.compactButtonPadding;
        const bool headerScaleChanged =
            state.headerScalePercent != m_settingsState.headerScalePercent;
        const bool commandtabScaleChanged =
            state.commandtabScalePercent != m_settingsState.commandtabScalePercent;
        const bool panelDropdownModeChanged =
            state.panelDropdownModeEnabled != m_settingsState.panelDropdownModeEnabled
            || state.panelDropdownPrimaryRecent != m_settingsState.panelDropdownPrimaryRecent
            || state.panelDropdownRecentToolCount != m_settingsState.panelDropdownRecentToolCount
            || state.panelDropdownPopupColumns != m_settingsState.panelDropdownPopupColumns
            || state.panelDropdownPopupShowText != m_settingsState.panelDropdownPopupShowText
            || state.panelDropdownPopupIconSize != m_settingsState.panelDropdownPopupIconSize;
        const bool ribbonAutoHideChanged =
            state.ribbonAutoHide != m_settingsState.ribbonAutoHide;
        const bool ribbonHoverTabChanged =
            state.ribbonHoverTab != m_settingsState.ribbonHoverTab;
        const bool tabClickPopupModeChanged =
            state.tabClickPopupMode != m_settingsState.tabClickPopupMode;

        m_settingsState = state;
        m_model.settings = state;
        for (auto& workbench : m_workbenchEntries) {
            applySettingsToWorkbench(workbench, state);
        }
        for (auto& workbench : m_model.workbenches) {
            applySettingsToWorkbench(workbench, state);
        }

        if (themeOverrideChanged) {
            m_theme = normalizeShellTheme(applySettingsThemeOverrides(m_baseTheme, state));
            m_model.theme = m_theme;
            applyThemeStyleSheet();
            updateBrandWidget();
        }

        if (headerScaleChanged) {
            applyShellDisplayScaleMetrics();
            rebuildHeaderCommandWidgets();
        }

        if (m_settingsDialog != nullptr) {
            if (themeOverrideChanged) {
                m_settingsDialog->setTheme(m_theme);
            }
            m_settingsDialog->setState(state);
        }
        if (m_customizationDialog != nullptr) {
            if (themeOverrideChanged) {
                m_customizationDialog->setTheme(m_theme);
            }
            m_customizationDialog->setModel(m_model);
        }

        if (ribbonAutoHideChanged) {
            if (state.ribbonAutoHide && !m_ribbonCollapsed) {
                setRibbonCollapsed(true, true);
            } else if (!state.ribbonAutoHide && m_ribbonCollapsed) {
                setRibbonCollapsed(false, true);
            }
        }
        if (tabClickPopupModeChanged && state.tabClickPopupMode && !m_ribbonCollapsed) {
            setRibbonCollapsed(true, true);
        }
        updateCollapseButton();

        if (ribbonHoverTabChanged && !state.ribbonHoverTab) {
            m_hoverTabPending = -1;
            if (m_hoverTabTimer != nullptr) {
                m_hoverTabTimer->stop();
            }
        }

        if (
            !themeOverrideChanged
            && !textVisibilityChanged
            && !iconOnlySizeChanged
            && !compactLayoutChanged
            && !headerScaleChanged
            && !commandtabScaleChanged
            && !panelDropdownModeChanged
            && !tabClickPopupModeChanged
        ) {
            return true;
        }

        if (
            !themeOverrideChanged
            && !textVisibilityChanged
            && !iconOnlySizeChanged
            && !compactLayoutChanged
            && !commandtabScaleChanged
            && !panelDropdownModeChanged
            && !tabClickPopupModeChanged
        ) {
            updateGeometry();
            if (m_contentChangedHandler) {
                m_contentChangedHandler();
            }
            return true;
        }

        for (auto& pageState : m_pageBuildStates) {
            if (pageState.layout != nullptr) {
                pageState.layout->setContentsMargins(panelContainerMargins());
                pageState.layout->setSpacing(panelContainerSpacing());
            }
        }

        // Rebuild every existing tab page so settings changes are visible everywhere.
        for (int index = 0; index < m_pageBuildStates.size(); ++index) {
            resetWorkbenchPage(index);
            if (index >= 0 && index < m_workbenchPagesDirty.size()) {
                m_workbenchPagesDirty[index] = false;
            }
        }

        const int currentIndex = m_tabBar != nullptr ? m_tabBar->currentIndex() : -1;
        if (currentIndex >= 0) {
            ensureWorkbenchPage(currentIndex);
            if (m_stack != nullptr) {
                m_stack->setCurrentIndex(currentIndex);
            }
        }

        updateGeometry();
        if (m_contentChangedHandler) {
            m_contentChangedHandler();
        }
        return true;
    }

    bool upsertWorkbench(const CommandTabWorkbenchEntry& workbench, bool activate)
    {
        if (workbench.id.isEmpty()) {
            return false;
        }
        const QString normalizedWorkbenchId = workbench.id.trimmed().toLower();
        const QString normalizedWorkbenchTitle = workbench.title.trimmed().toLower();
        if (
            normalizedWorkbenchId == QStringLiteral("noneworkbench")
            || normalizedWorkbenchId == QStringLiteral("<none>")
            || normalizedWorkbenchId == QStringLiteral("none")
            || normalizedWorkbenchTitle == QStringLiteral("<none>")
            || normalizedWorkbenchTitle == QStringLiteral("none")
        ) {
            return false;
        }

        const qsizetype existingIndex = m_workbenchIds.indexOf(workbench.id);
        int index = static_cast<int>(existingIndex);

        if (existingIndex < 0) {
            index = static_cast<int>(m_workbenchEntries.size());
            m_workbenchEntries.push_back(workbench);
            m_workbenchIds.push_back(workbench.id);
            m_workbenchPagesBuilt.push_back(false);
            m_workbenchPagesDirty.push_back(false);
            m_pageBuildStates.push_back(PageBuildState());
            m_model.workbenches.push_back(workbench);
            const QIcon workbenchIcon = resolveWorkbenchIcon(workbench);
            m_tabBar->addTab(workbenchIcon, workbench.title);
            m_tabBar->setTabToolTip(index, workbench.title);
            auto* placeholder = new QWidget(m_stack);
            placeholder->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
            m_stack->addWidget(placeholder);
        } else {
            m_workbenchEntries[index] = workbench;
            if (index >= 0 && index < m_model.workbenches.size()) {
                m_model.workbenches[index] = workbench;
            }
            const QIcon workbenchIcon = resolveWorkbenchIcon(workbench);
            m_tabBar->setTabIcon(index, workbenchIcon);
            m_tabBar->setTabText(index, workbench.title);
            m_tabBar->setTabToolTip(index, workbench.title);
            resetWorkbenchPage(index);
            if (index >= 0 && index < m_workbenchPagesDirty.size()) {
                m_workbenchPagesDirty[index] = false;
            }
        }

        if (activate) {
            ensureWorkbenchPage(index);
            QSignalBlocker tabBlocker(m_tabBar);
            m_tabBar->setCurrentIndex(index);
            m_stack->setCurrentIndex(index);
            m_model.activeWorkbenchId = workbench.id;
        }
        applyTabTextColors();
        updateTabNavigationButtons();
        if (m_customizationDialog != nullptr) {
            m_customizationDialog->setModel(m_model);
        }
        scheduleBackgroundPageWarmup();
        scheduleCommandIconRefresh(30, 2);
        return true;
    }

    void applyTabTextColors()
    {
        if (m_tabBar == nullptr) {
            return;
        }

        const int currentIndex = m_tabBar->currentIndex();
        for (int tabIndex = 0; tabIndex < m_tabBar->count(); ++tabIndex) {
            const bool selected = tabIndex == currentIndex;
            const QColor targetColor = selected
                ? (m_tabSelectedTextColor.isValid() ? m_tabSelectedTextColor : m_tabTextColor)
                : m_tabTextColor;
            if (targetColor.isValid()) {
                m_tabBar->setTabTextColor(tabIndex, targetColor);
            }
        }
    }

    void updateTabNavigationButtons()
    {
        if (m_tabBar == nullptr) {
            return;
        }
        if (m_updatingTabScrollButtonLayout) {
            return;
        }

        QToolButton* leftButton = nullptr;
        QToolButton* rightButton = nullptr;
        const QList<QToolButton*> childButtons = m_tabBar->findChildren<QToolButton*>();
        for (QToolButton* button : childButtons) {
            if (button == nullptr) {
                continue;
            }
            if (button->arrowType() == Qt::LeftArrow) {
                if (leftButton == nullptr || !leftButton->isVisible()) {
                    leftButton = button;
                }
            } else if (button->arrowType() == Qt::RightArrow) {
                if (rightButton == nullptr || !rightButton->isVisible()) {
                    rightButton = button;
                }
            }
        }
        if (leftButton == nullptr || rightButton == nullptr) {
            return;
        }

        m_updatingTabScrollButtonLayout = true;

        const bool showButtons = leftButton->isVisible() || rightButton->isVisible();
        if (!showButtons) {
            m_tabBar->setContentsMargins(0, 0, 0, 0);
            m_updatingTabScrollButtonLayout = false;
            return;
        }

        const int navHeight = std::max(scaledHeaderPx(20), m_tabBar->height());
        const int navWidth = std::max(scaledHeaderPx(20), scaledHeaderPx(32));
        leftButton->setFixedSize(navWidth, navHeight);
        rightButton->setFixedSize(navWidth, navHeight);

        const int reservePadding = scaledHeaderPx(3);
        const int leftReserve = leftButton->width() + reservePadding;
        const int rightReserve = rightButton->width() + reservePadding;
        m_tabBar->setContentsMargins(leftReserve, 0, rightReserve, 0);

        leftButton->move(0, 0);
        rightButton->move(std::max(0, m_tabBar->width() - rightButton->width()), 0);

        struct TabBounds {
            QRect first;
            QRect last;
        };
        auto firstLastVisibleTabBounds = [this]() -> TabBounds {
            QRect firstRect;
            QRect lastRect;
            for (int i = 0; i < m_tabBar->count(); ++i) {
                const QRect tabRect = m_tabBar->tabRect(i);
                if (!tabRect.isValid() || tabRect.width() <= 0 || tabRect.height() <= 0) {
                    continue;
                }
                if (firstRect.isNull()) {
                    firstRect = tabRect;
                }
                lastRect = tabRect;
            }
            return TabBounds{firstRect, lastRect};
        };

        const auto tabBounds = firstLastVisibleTabBounds();
        const QRect firstTabRect = tabBounds.first;
        const QRect lastTabRect = tabBounds.last;
        if (firstTabRect.isValid() && lastTabRect.isValid()) {
            const int gap = scaledHeaderPx(4);
            const int leftOverlap = (leftButton->geometry().right() + gap) - firstTabRect.left();
            const int rightOverlap = (lastTabRect.right() + gap) - rightButton->geometry().left();
            if (leftOverlap > 0 || rightOverlap > 0) {
                const int symmetricReserveBoost = std::max(
                    std::max(0, leftOverlap),
                    std::max(0, rightOverlap)
                );
                const int correctedLeftReserve = leftReserve + symmetricReserveBoost;
                const int correctedRightReserve = rightReserve + symmetricReserveBoost;
                m_tabBar->setContentsMargins(correctedLeftReserve, 0, correctedRightReserve, 0);
            }
        }

        leftButton->raise();
        rightButton->raise();
        m_updatingTabScrollButtonLayout = false;
    }

private:
    class CommandTabPanelSplitButtonWidget final : public QWidget
    {
    public:
        explicit CommandTabPanelSplitButtonWidget(
            QWidget* primaryButton,
            QToolButton* expandButton,
            int displayScalePercent = 100,
            QWidget* parent = nullptr
        )
            : QWidget(parent)
            , m_primaryButton(primaryButton)
            , m_expandButton(expandButton)
            , m_displayScalePercent(std::clamp(displayScalePercent, 60, 140))
        {
            setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            setAttribute(Qt::WA_StyledBackground, false);
            setProperty("commandtabCompactIconOnly", true);

            if (m_primaryButton != nullptr) {
                m_primaryButton->setParent(this);
                m_primaryButton->show();
            }
            if (m_expandButton != nullptr) {
                m_expandButton->setParent(this);
                m_expandButton->show();
            }

            const int preferredWidth = std::max(
                scaledPx(30),
                std::max(
                    m_primaryButton != nullptr ? m_primaryButton->sizeHint().width() : 0,
                    m_expandButton != nullptr ? m_expandButton->sizeHint().width() : 0
                )
            );
            const int primaryHeight = std::clamp(
                m_primaryButton != nullptr ? m_primaryButton->sizeHint().height() : scaledPx(30),
                scaledPx(24),
                scaledPx(36)
            );
            const int expandHeight = std::max(scaledPx(12), primaryHeight / 2);
            m_preferredSize = QSize(
                preferredWidth,
                std::clamp(primaryHeight + expandHeight, scaledPx(42), scaledPx(56))
            );
            setMinimumSize(QSize(0, 0));
        }

        QSize sizeHint() const override
        {
            return m_preferredSize;
        }

        QSize minimumSizeHint() const override
        {
            return m_preferredSize;
        }

    protected:
        void resizeEvent(QResizeEvent* event) override
        {
            QWidget::resizeEvent(event);
            const int totalWidth = std::max(1, width());
            const int totalHeight = std::max(1, height());

            int expandHeight = std::max(scaledPx(12), totalHeight / 3);
            if (expandHeight > totalHeight - scaledPx(18)) {
                expandHeight = std::max(scaledPx(10), totalHeight - scaledPx(18));
            }
            const int primaryHeight = std::max(scaledPx(18), totalHeight - expandHeight);
            const int finalExpandHeight = std::max(scaledPx(8), totalHeight - primaryHeight);

            if (m_primaryButton != nullptr) {
                m_primaryButton->setGeometry(0, 0, totalWidth, primaryHeight);
            }
            if (m_expandButton != nullptr) {
                m_expandButton->setGeometry(0, primaryHeight, totalWidth, finalExpandHeight);
                const int iconEdge = std::clamp(
                    std::min(totalWidth - scaledPx(6), finalExpandHeight - scaledPx(4)),
                    scaledPx(8),
                    scaledPx(14)
                );
                m_expandButton->setIconSize(QSize(iconEdge, iconEdge));
            }
        }

    private:
        QWidget* m_primaryButton = nullptr;
        QToolButton* m_expandButton = nullptr;
        int m_displayScalePercent = 100;
        QSize m_preferredSize = QSize(30, 44);

        qreal displayScaleFactor() const
        {
            return static_cast<qreal>(std::clamp(m_displayScalePercent, 60, 140)) / 100.0;
        }

        int scaledPx(int value) const
        {
            return std::max(1, qRound(static_cast<qreal>(value) * displayScaleFactor()));
        }
    };

    QIcon resolveWorkbenchIcon(const CommandTabWorkbenchEntry& workbench) const
    {
        const QString explicitIconPath = workbench.iconPath.trimmed();
        const QString normalizedExplicitIcon = explicitIconPath.toLower();
        if (
            !explicitIconPath.isEmpty()
            && normalizedExplicitIcon != QStringLiteral("noneworkbench")
            && normalizedExplicitIcon != QStringLiteral("none")
        ) {
            QIcon icon = loadIconFromSource(explicitIconPath);
            if (!icon.isNull()) {
                return icon;
            }

            const QString staleStemCandidate = staleIconStemCandidate(explicitIconPath);
            if (!staleStemCandidate.isEmpty()) {
                icon = loadIconFromSource(staleStemCandidate);
                if (!icon.isNull()) {
                    commandtabDebugLog(
                        QStringLiteral("icon-fallback"),
                        QStringLiteral("Recovered stale workbench icon for '%1' using stem '%2'")
                            .arg(workbench.id, staleStemCandidate)
                    );
                    return icon;
                }
            }
        }

        for (const auto& candidate : workbenchIconCandidates(workbench.id, workbench.title)) {
            const QString normalizedCandidate = candidate.trimmed().toLower();
            if (
                normalizedCandidate.isEmpty()
                || normalizedCandidate == QStringLiteral("noneworkbench")
                || normalizedCandidate.endsWith(QStringLiteral("/noneworkbench.svg"))
                || normalizedCandidate.endsWith(QStringLiteral("/noneworkbench.png"))
            ) {
                continue;
            }
            QIcon icon = loadIconFromSource(candidate);
            if (!icon.isNull()) {
                return icon;
            }
        }
        const QColor badgeBackground = blendColors(
            popupMenuSurfaceColor(),
            m_theme.panelBodyAccent,
            m_theme.isDark ? 0.42 : 0.30
        );
        const QColor badgeForeground = ensureReadableTextColor(
            badgeBackground,
            m_theme.buttonText,
            4.5
        );
        const QString fallbackLabel = workbench.title.trimmed().isEmpty() ? workbench.id : workbench.title;
        return buildMonogramIcon(fallbackLabel, workbenchTabIconSize(), badgeBackground, badgeForeground);
    }

    void refreshWorkbenchTabIcons()
    {
        if (m_tabBar == nullptr) {
            return;
        }

        const int tabCount = std::min(m_tabBar->count(), static_cast<int>(m_workbenchEntries.size()));
        if (tabCount <= 0) {
            return;
        }

        for (int tabIndex = 0; tabIndex < tabCount; ++tabIndex) {
            const QIcon icon = resolveWorkbenchIcon(m_workbenchEntries.at(tabIndex));
            if (!icon.isNull()) {
                m_tabBar->setTabIcon(tabIndex, icon);
            }
        }
    }

    void scheduleWorkbenchTabIconRefresh(int delayMs = 220)
    {
        if (m_tabBar == nullptr) {
            return;
        }
        if (m_tabIconRefreshTimer == nullptr) {
            m_tabIconRefreshTimer = new QTimer(this);
            m_tabIconRefreshTimer->setSingleShot(true);
            connect(m_tabIconRefreshTimer, &QTimer::timeout, this, [this]() {
                refreshWorkbenchTabIcons();
            });
        }
        m_tabIconRefreshTimer->start(std::clamp(delayMs, 0, 2400));
    }

    void refreshCommandIconsFromActions()
    {
        invalidateActionLookup();
        rebuildActionLookup();

        QList<QWidget*> scanRoots;
        scanRoots.push_back(this);
        if (m_topBarWidget != nullptr) {
            scanRoots.push_back(m_topBarWidget);
        }
        if (m_stack != nullptr && m_stack->currentWidget() != nullptr) {
            scanRoots.push_back(m_stack->currentWidget());
        }

        QSet<QWidget*> visitedRoots;
        for (QWidget* root : scanRoots) {
            if (root == nullptr || visitedRoots.contains(root)) {
                continue;
            }
            visitedRoots.insert(root);

            const auto scopedWidgets = root->findChildren<QWidget*>();
            for (auto* scopedWidget : scopedWidgets) {
                auto* commandButton = dynamic_cast<CommandTabCommandButton*>(scopedWidget);
                if (commandButton == nullptr) {
                    continue;
                }
                const QString commandId = commandButton->property("commandtabCommandId").toString().trimmed();
                if (commandId.isEmpty()) {
                    continue;
                }
                QAction* sourceAction = resolveActionForCommandId(commandId);
                if (sourceAction == nullptr || sourceAction->icon().isNull()) {
                    continue;
                }
                commandButton->setIconOverride(sourceAction->icon());
            }

            const auto toolButtons = root->findChildren<QToolButton*>();
            for (auto* toolButton : toolButtons) {
                if (toolButton == nullptr) {
                    continue;
                }
                const QString commandId = toolButton->property("commandtabCommandId").toString().trimmed();
                if (commandId.isEmpty()) {
                    continue;
                }
                QAction* sourceAction = resolveActionForCommandId(commandId);
                if (sourceAction == nullptr || sourceAction->icon().isNull()) {
                    continue;
                }
                toolButton->setIcon(sourceAction->icon());

                const QVariant preferredStyleValue = toolButton->property("commandtabPreferredToolButtonStyle");
                if (preferredStyleValue.isValid()) {
                    const int preferredStyle = preferredStyleValue.toInt();
                    toolButton->setToolButtonStyle(static_cast<Qt::ToolButtonStyle>(preferredStyle));
                    if (preferredStyle != static_cast<int>(Qt::ToolButtonTextUnderIcon)) {
                        toolButton->setText(QString());
                    }
                }
            }
        }
    }

    void scheduleCommandIconRefresh(int delayMs = 220, int passes = 1)
    {
        if (passes <= 0) {
            return;
        }
        m_pendingCommandIconRefreshPasses = std::max(m_pendingCommandIconRefreshPasses, passes);
        if (m_commandIconRefreshTimer == nullptr) {
            m_commandIconRefreshTimer = new QTimer(this);
            m_commandIconRefreshTimer->setSingleShot(true);
            connect(m_commandIconRefreshTimer, &QTimer::timeout, this, [this]() {
                refreshCommandIconsFromActions();
                if (m_pendingCommandIconRefreshPasses > 1) {
                    --m_pendingCommandIconRefreshPasses;
                    m_commandIconRefreshTimer->start(120);
                } else {
                    m_pendingCommandIconRefreshPasses = 0;
                }
            });
        }
        m_commandIconRefreshTimer->start(std::clamp(delayMs, 0, 3000));
    }

    int headerScalePercent() const
    {
        return std::clamp(m_settingsState.headerScalePercent, 60, 140);
    }

    int commandtabScalePercent() const
    {
        return std::clamp(m_settingsState.commandtabScalePercent, 60, 140);
    }

    qreal headerScaleFactor() const
    {
        return static_cast<qreal>(headerScalePercent()) / 100.0;
    }

    qreal commandtabScaleFactor() const
    {
        return static_cast<qreal>(commandtabScalePercent()) / 100.0;
    }

    int scaledHeaderPx(int value) const
    {
        return std::max(1, qRound(static_cast<qreal>(value) * headerScaleFactor()));
    }

    int scaledPx(int value) const
    {
        return std::max(1, qRound(static_cast<qreal>(value) * commandtabScaleFactor()));
    }

    void applyShellDisplayScaleMetrics()
    {
        if (m_applyingDisplayScaleMetrics) {
            return;
        }
        m_applyingDisplayScaleMetrics = true;

        const int widthHint = std::max(width(), m_topBarWidget != nullptr ? m_topBarWidget->width() : 0);
        int adaptiveDensity = 0;
        if (widthHint > 0) {
            if (widthHint < scaledHeaderPx(980)) {
                adaptiveDensity = 2;
            } else if (widthHint < scaledHeaderPx(1240)) {
                adaptiveDensity = 1;
            }
        }
        m_adaptiveDensityLevel = adaptiveDensity;

        auto denseSubtract = [adaptiveDensity](int base, int d1, int d2) {
            if (adaptiveDensity >= 2) {
                return std::max(0, base - d2);
            }
            if (adaptiveDensity == 1) {
                return std::max(0, base - d1);
            }
            return base;
        };

        if (auto* outerLayout = qobject_cast<QVBoxLayout*>(layout())) {
            outerLayout->setContentsMargins(
                scaledHeaderPx(denseSubtract(6, 1, 2)),
                scaledHeaderPx(denseSubtract(4, 1, 1)),
                scaledHeaderPx(denseSubtract(6, 1, 2)),
                scaledHeaderPx(denseSubtract(4, 1, 1))
            );
            outerLayout->setSpacing(scaledHeaderPx(denseSubtract(3, 1, 2)));
        }

        if (m_topBarWidget != nullptr) {
            m_topBarWidget->setMinimumHeight(scaledHeaderPx(denseSubtract(42, 3, 5)));
        }
        if (m_topBarLayout != nullptr) {
            m_topBarLayout->setContentsMargins(
                scaledHeaderPx(denseSubtract(8, 2, 3)),
                scaledHeaderPx(denseSubtract(4, 1, 2)),
                scaledHeaderPx(denseSubtract(8, 2, 3)),
                scaledHeaderPx(denseSubtract(4, 1, 2))
            );
            m_topBarLayout->setSpacing(scaledHeaderPx(denseSubtract(6, 2, 3)));
        }

        if (m_quickAccessLayout != nullptr) {
            m_quickAccessLayout->setContentsMargins(
                scaledHeaderPx(denseSubtract(4, 1, 2)),
                scaledHeaderPx(denseSubtract(3, 1, 2)),
                scaledHeaderPx(denseSubtract(4, 1, 2)),
                scaledHeaderPx(denseSubtract(3, 1, 2))
            );
            m_quickAccessLayout->setSpacing(scaledHeaderPx(denseSubtract(3, 1, 2)));
        }
        if (m_utilityLayout != nullptr) {
            m_utilityLayout->setContentsMargins(
                scaledHeaderPx(denseSubtract(4, 1, 2)),
                scaledHeaderPx(denseSubtract(3, 1, 2)),
                scaledHeaderPx(denseSubtract(4, 1, 2)),
                scaledHeaderPx(denseSubtract(3, 1, 2))
            );
            m_utilityLayout->setSpacing(scaledHeaderPx(denseSubtract(5, 2, 3)));
        }
        if (m_brandWidget != nullptr) {
            m_brandWidget->setVisible(adaptiveDensity < 2);
            m_brandWidget->setFixedSize(
                scaledHeaderPx(denseSubtract(38, 4, 8)),
                scaledHeaderPx(denseSubtract(30, 3, 6))
            );
        }
        if (m_tabBar != nullptr) {
            m_tabBar->setIconSize(workbenchTabIconSize());
            m_tabBar->setProperty("commandtabDensity", adaptiveDensity);
            QFont tabFont = m_tabBar->font();
            tabFont.setPointSize(std::clamp(scaledHeaderPx(denseSubtract(10, 1, 2)), 7, 22));
            tabFont.setBold(true);
            m_tabBar->setFont(tabFont);
        }

        if (m_quickAccessWidget != nullptr) {
            const auto quickButtons = m_quickAccessWidget->findChildren<QToolButton*>(
                QString(),
                Qt::FindDirectChildrenOnly
            );
            const int quickEdge = scaledHeaderPx(denseSubtract(30, 2, 4));
            for (QToolButton* button : quickButtons) {
                if (button == nullptr) {
                    continue;
                }
                button->setFixedSize(quickEdge, quickEdge);
            }
        }

        if (m_utilityWidget != nullptr) {
            const auto utilityButtons = m_utilityWidget->findChildren<QToolButton*>(
                QString(),
                Qt::FindDirectChildrenOnly
            );
            const int utilityMinWidth = scaledHeaderPx(denseSubtract(58, 8, 14));
            const int utilityHeight = scaledHeaderPx(denseSubtract(24, 2, 4));
            for (QToolButton* button : utilityButtons) {
                if (button == nullptr) {
                    continue;
                }
                button->setMinimumWidth(utilityMinWidth);
                button->setFixedHeight(std::max(utilityHeight, button->fontMetrics().lineSpacing() + scaledHeaderPx(3)));
            }
        }

        for (auto& pageState : m_pageBuildStates) {
            if (pageState.layout == nullptr) {
                continue;
            }
            pageState.layout->setContentsMargins(panelContainerMargins());
            pageState.layout->setSpacing(panelContainerSpacing());
        }

        updateTabNavigationButtons();
        updateBrandWidget();
        updateGeometry();
        m_applyingDisplayScaleMetrics = false;
    }

    void rebuildHeaderCommandWidgets()
    {
        clearLayout(m_quickAccessLayout);
        clearLayout(m_utilityLayout);
        if (m_collapseButton != nullptr) {
            if (m_topBarLayout != nullptr) {
                m_topBarLayout->removeWidget(m_collapseButton);
            }
            m_collapseButton->deleteLater();
            m_collapseButton = nullptr;
        }

        if (m_tabBar != nullptr) {
            m_tabBar->setIconSize(workbenchTabIconSize());
        }

        if (
            !isCommandInQuickAccess(QStringLiteral("__commandtab_toggle_grid__"))
            && !isCommandInQuickAccess(QStringLiteral("Draft_ToggleGrid"))
        ) {
            CommandTabCommandEntry gridQuickCommand;
            gridQuickCommand.type = QStringLiteral("command");
            gridQuickCommand.id = QStringLiteral("__commandtab_toggle_grid__");
            gridQuickCommand.text = translatedShellText("Toggle grid");
            gridQuickCommand.size = QStringLiteral("small");
            // Prefer a bundled stable grid glyph at startup; runtime QAction
            // icon still wins later when available.
            gridQuickCommand.iconPath = QStringLiteral("Sketcher_GridToggle_Deactivated.svg");
            if (auto* gridButton = createCommandButton(gridQuickCommand, true)) {
                m_quickAccessLayout->addWidget(gridButton);
            }
        }
        for (const auto& command : m_model.quickAccess) {
            auto* button = createCommandButton(command, true);
            if (button != nullptr) {
                m_quickAccessLayout->addWidget(button);
            }
        }
        if (auto* preferencesButton = createUtilityButton(
                QStringLiteral("__commandtab_preferences__"),
                translatedShellText("Preferences"),
                translatedShellText("CommandTab preferences")
            )) {
            m_utilityLayout->addWidget(preferencesButton);
        }
        if (auto* designButton = createUtilityButton(
                QStringLiteral("__commandtab_design__"),
                translatedShellText("Design"),
                translatedShellText("CommandTab design"),
                [this]() { openCustomizationDialog(currentWorkbenchId(), QString()); }
            )) {
            m_utilityLayout->addWidget(designButton);
        }
        if (m_quickAccessWidget != nullptr) {
            m_quickAccessWidget->setVisible(m_quickAccessLayout->count() > 0);
        }
        if (m_utilityWidget != nullptr) {
            m_utilityWidget->setVisible(m_utilityLayout->count() > 0);
        }
        if (m_topBarWidget != nullptr) {
            m_topBarWidget->setVisible(true);
        }

        // Keep the legacy dedicated grid button hidden: the grid toggle now
        // lives in quick access for a more consistent header layout.
        if (m_gridToggleButton != nullptr) {
            m_gridToggleButton->setVisible(false);
        }

        m_collapseButton = new QToolButton(m_topBarWidget);
        m_collapseButton->setObjectName(QStringLiteral("CommandTabCollapseButton"));
        m_collapseButton->setAutoRaise(true);
        m_collapseButton->setFixedSize(scaledHeaderPx(22), scaledHeaderPx(22));
        QFont collapseFont = m_collapseButton->font();
        collapseFont.setPointSize(7);
        m_collapseButton->setFont(collapseFont);
        m_topBarLayout->addWidget(m_collapseButton, 0, Qt::AlignVCenter);
        updateCollapseButton();
        connect(m_collapseButton, &QToolButton::clicked, this, [this]() {
            cancelAutoHideCountdown();
            setRibbonCollapsed(!m_ribbonCollapsed, true);
        });
    }

    int toolbarReferenceIconSize() const
    {
        int size = 0;
        if (const auto* mainWindow = qobject_cast<const QMainWindow*>(window())) {
            size = mainWindow->iconSize().width();
        }
        if (size < 16) {
            size = style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, this);
        }
        return std::clamp(size, 16, 32);
    }

    QSize quickAccessIconSize() const
    {
        const int baseSize = std::clamp(toolbarReferenceIconSize() - 2, 18, 24);
        const int size = std::clamp(scaledHeaderPx(baseSize), 12, 40);
        return QSize(size, size);
    }

    QSize workbenchTabIconSize() const
    {
        const int baseSize = std::clamp(toolbarReferenceIconSize() - 3, 18, 22);
        const int size = std::clamp(scaledHeaderPx(baseSize), 12, 36);
        return QSize(size, size);
    }

    QIcon buildThemedArrowIcon(const QColor& color, const QSize& size) const
    {
        const int width = std::max(4, size.width());
        const int height = std::max(4, size.height());
        QPixmap pixmap(width, height);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);

        const int insetX = std::max(1, width / 4);
        const int topY = std::max(1, height / 3);
        const int bottomY = std::max(topY + 1, height - std::max(1, height / 4));
        QPolygon triangle;
        triangle << QPoint(insetX, topY)
                 << QPoint(width - insetX, topY)
                 << QPoint(width / 2, bottomY);
        painter.drawPolygon(triangle);

        return QIcon(pixmap);
    }

    QIcon buildMonogramIcon(
        const QString& rawText,
        const QSize& requestedSize,
        const QColor& backgroundColor,
        const QColor& foregroundColor
    ) const
    {
        const int edge = std::max(14, std::max(requestedSize.width(), requestedSize.height()));
        QPixmap pixmap(edge, edge);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(backgroundColor);
        painter.drawRoundedRect(QRectF(0.5, 0.5, edge - 1.0, edge - 1.0), edge * 0.22, edge * 0.22);

        QString monogram = rawText.simplified();
        if (monogram.isEmpty()) {
            monogram = QStringLiteral("•");
        } else {
            monogram = monogram.left(1).toUpper();
        }

        QFont font = painter.font();
        font.setBold(true);
        font.setPointSize(std::max(7, edge / 2));
        painter.setFont(font);
        painter.setPen(foregroundColor);
        painter.drawText(QRect(0, 0, edge, edge), Qt::AlignCenter, monogram);
        return QIcon(pixmap);
    }

    void updateBrandWidget()
    {
        if (m_brandWidget == nullptr) {
            return;
        }

        const QIcon logoIcon = loadIconFromSource(nativeHeaderLogoResource(m_theme));
        if (logoIcon.isNull()) {
            m_brandWidget->clear();
            return;
        }

        m_brandWidget->setPixmap(logoIcon.pixmap(QSize(scaledHeaderPx(21), scaledHeaderPx(21)), QIcon::Normal, QIcon::Off));
    }

    QColor popupMenuSurfaceColor() const
    {
        return withAlpha(
            blendColors(m_theme.panelCardBackground, m_theme.panelFooterBackground, 0.22),
            m_theme.isDark ? 204 : 224
        );
    }

    QColor popupMenuHoverColor() const
    {
        return withAlpha(
            blendColors(m_theme.panelFooterBackground, m_theme.panelCardBackground, 0.38),
            m_theme.isDark ? 186 : 210
        );
    }

    QColor popupMenuBorderColor() const
    {
        return withAlpha(
            blendColors(m_theme.panelCardBorder, m_theme.shellBorder, 0.20),
            224
        );
    }

    QColor popupMenuFocusBorderColor() const
    {
        return withAlpha(m_theme.buttonFocusBorder, 232);
    }

    QColor popupMenuTextColor() const
    {
        if (m_theme.forceTextColor && m_theme.buttonText.isValid()) {
            return m_theme.buttonText;
        }
        return ensureReadableTextColor(popupMenuSurfaceColor(), m_theme.buttonText, 4.8);
    }

    QColor popupMenuDisabledTextColor() const
    {
        if (m_theme.forceTextColor && m_theme.buttonText.isValid()) {
            return m_theme.buttonText;
        }
        const QColor text = popupMenuTextColor();
        const QColor softened = blendColors(
            text,
            m_theme.shellBackground,
            m_theme.isDark ? 0.30 : 0.42
        );
        return ensureReadableTextColor(popupMenuSurfaceColor(), softened, 4.2);
    }

    void applyPopupMenuTheme(QMenu* menu) const
    {
        if (menu == nullptr) {
            return;
        }

        menu->setWindowFlag(Qt::FramelessWindowHint, true);
        menu->setAttribute(Qt::WA_TranslucentBackground, true);
        menu->setAttribute(Qt::WA_StyledBackground, true);

        const QColor menuSurface = popupMenuSurfaceColor();
        const QColor menuHover = popupMenuHoverColor();
        const QColor menuBorder = popupMenuBorderColor();
        const QColor menuText = popupMenuTextColor();
        const QColor disabledText = popupMenuDisabledTextColor();
        const QColor focusBorder = popupMenuFocusBorderColor();

        QPalette menuPalette = menu->palette();
        const QPalette::ColorGroup activeGroups[] = {
            QPalette::Active,
            QPalette::Inactive,
        };
        for (QPalette::ColorGroup group : activeGroups) {
            menuPalette.setColor(group, QPalette::Window, menuSurface);
            menuPalette.setColor(group, QPalette::Base, menuSurface);
            menuPalette.setColor(group, QPalette::Button, menuSurface);
            menuPalette.setColor(group, QPalette::WindowText, menuText);
            menuPalette.setColor(group, QPalette::Text, menuText);
            menuPalette.setColor(group, QPalette::ButtonText, menuText);
            menuPalette.setColor(group, QPalette::Highlight, menuHover);
            menuPalette.setColor(group, QPalette::HighlightedText, menuText);
        }
        menuPalette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
        menuPalette.setColor(QPalette::Disabled, QPalette::Text, disabledText);
        menuPalette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
        menuPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledText);
        menu->setPalette(menuPalette);

        menu->setStyleSheet(
            QStringLiteral(R"(
QMenu {
    background: %1;
    border: 1px solid %2;
    border-radius: 10px;
    padding: 6px;
    margin: 0px;
}
QMenu::item {
    color: %3;
    background: transparent;
    padding: 6px 10px;
    margin: 1px 3px;
    border: 1px solid transparent;
    border-radius: 7px;
}
QMenu::item:selected {
    background: %4;
    border: 1px solid %5;
}
QMenu::item:disabled {
    color: %6;
}
QMenu::separator {
    height: 6px;
    margin: 0px;
    background: transparent;
}
QWidget#CommandTabPanelPopupGrid {
    background: %1;
    border: none;
    border-radius: 8px;
}
)")
                .arg(menuSurface.name(QColor::HexArgb))
                .arg(menuBorder.name(QColor::HexArgb))
                .arg(menuText.name(QColor::HexArgb))
                .arg(menuHover.name(QColor::HexArgb))
                .arg(focusBorder.name(QColor::HexArgb))
                .arg(disabledText.name(QColor::HexArgb))
        );
    }

    void applyPanelPopupToolButtonTheme(QToolButton* button) const
    {
        if (button == nullptr) {
            return;
        }

        const QColor buttonText = popupMenuTextColor();
        const QColor disabledText = popupMenuDisabledTextColor();
        const QColor hoverSurface = popupMenuHoverColor();
        const QColor borderColor = popupMenuBorderColor();
        const QColor focusBorder = popupMenuFocusBorderColor();

        QPalette buttonPalette = button->palette();
        buttonPalette.setColor(QPalette::Active, QPalette::ButtonText, buttonText);
        buttonPalette.setColor(QPalette::Inactive, QPalette::ButtonText, buttonText);
        buttonPalette.setColor(QPalette::Active, QPalette::WindowText, buttonText);
        buttonPalette.setColor(QPalette::Inactive, QPalette::WindowText, buttonText);
        buttonPalette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
        buttonPalette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
        buttonPalette.setColor(QPalette::Disabled, QPalette::Text, disabledText);
        button->setPalette(buttonPalette);

        button->setStyleSheet(
            QStringLiteral(R"(
QToolButton {
    color: %1;
    background: transparent;
    border: 1px solid transparent;
    border-radius: 6px;
    padding: 2px 3px;
}
QToolButton:hover {
    background: %2;
    border: 1px solid %3;
}
QToolButton:focus {
    border: 1px solid %4;
}
QToolButton:disabled {
    color: %5;
}
)")
                .arg(buttonText.name(QColor::HexArgb))
                .arg(hoverSurface.name(QColor::HexArgb))
                .arg(borderColor.name(QColor::HexArgb))
                .arg(focusBorder.name(QColor::HexArgb))
                .arg(disabledText.name(QColor::HexArgb))
        );
    }

    void applyThemeStyleSheet()
    {
        const bool forceTextColor = m_theme.forceTextColor && m_theme.buttonText.isValid();
        const QColor headerTopBase = blendColors(
            blendColors(m_theme.shellBackground, m_theme.panelCardBackground, 0.20),
            m_theme.tabAccent,
            m_theme.isDark ? 0.10 : 0.07
        );
        const QColor headerBottomBase = blendColors(
            blendColors(m_theme.shellBackground, m_theme.panelBodyBottom, 0.24),
            m_theme.tabAccent,
            m_theme.isDark ? 0.08 : 0.05
        );
        const QColor headerTop = withAlpha(headerTopBase, m_theme.isDark ? 186 : 218);
        const QColor headerBottom = withAlpha(headerBottomBase, m_theme.isDark ? 166 : 196);
        const QColor headerSurface = blendColors(headerTop, headerBottom, 0.50);
        const QColor headerLine = withAlpha(blendColors(m_theme.shellBorder, m_theme.panelCardBorder, 0.34), 194);
        const QColor sectionSurface = withAlpha(
            blendColors(m_theme.panelCardBackground, m_theme.quickBackground, 0.28),
            m_theme.isDark ? 108 : 164
        );
        const QColor hoverSurface = withAlpha(blendColors(m_theme.quickHoverBackground, m_theme.panelBodyTop, 0.24), 214);
        const QColor primaryText = m_theme.buttonText;
        const QColor preferredTitleText = m_theme.titleText.isValid() ? m_theme.titleText : primaryText;
        const QColor subtleText = forceTextColor
            ? primaryText
            : ensureReadableTextColor(headerSurface, preferredTitleText, 4.2);
        const QColor tabHoverSurface = withAlpha(
            blendColors(
                blendColors(m_theme.tabHoverBackground, m_theme.panelBodyTop, 0.18),
                m_theme.tabAccent,
                m_theme.isDark ? 0.26 : 0.18
            ),
            224
        );
        const QColor tabSelectedSeed = m_theme.isDark
            ? blendColors(
                blendColors(m_theme.tabSelectedBackground, m_theme.panelCardBackground, 0.36),
                m_theme.shellBackground,
                0.24
            )
            : blendColors(m_theme.tabSelectedBackground, m_theme.panelCardBackground, 0.16);
        const QColor tabSelectedSurface = withAlpha(
            blendColors(tabSelectedSeed, m_theme.tabAccent, m_theme.isDark ? 0.24 : 0.16),
            m_theme.isDark ? 220 : 242
        );
        const QColor tabText = forceTextColor
            ? primaryText
            : ensureReadableTextColor(headerSurface, subtleText, 4.3);
        const QColor tabSelectedText = forceTextColor
            ? primaryText
            : ensureReadableTextColor(
                tabSelectedSurface,
                preferredTitleText,
                6.2,
                QColor(QStringLiteral("#0f1823")),
                QColor(QStringLiteral("#f5f8fd"))
            );
        const QColor tabSelectedBorder = withAlpha(m_theme.tabAccent, m_theme.isDark ? 236 : 252);
        const QColor utilitySurface = withAlpha(sectionSurface, m_theme.isDark ? 114 : 178);
        const QColor utilityBorder = withAlpha(blendColors(m_theme.quickBorder, m_theme.shellBorder, 0.40), 188);
        const QColor brandSurfaceSeed = m_theme.isDark
            ? blendColors(
                blendColors(m_theme.tabSelectedBackground, m_theme.panelCardBackground, 0.44),
                m_theme.panelFooterBackground,
                0.26
            )
            : blendColors(m_theme.tabSelectedBackground, m_theme.panelFooterBackground, 0.24);
        const QColor brandSurface = withAlpha(brandSurfaceSeed, m_theme.isDark ? 220 : 238);
        const QColor brandBorder = withAlpha(
            blendColors(m_theme.tabSelectedBorder, m_theme.buttonActiveBorder, 0.42),
            m_theme.isDark ? 200 : 194
        );
        const QColor dividerColor = withAlpha(blendColors(m_theme.shellBorder, m_theme.tabAccent, 0.26), 138);
        const QColor cardBorder = withAlpha(blendColors(m_theme.panelCardBorder, headerLine, 0.18), 188);
        const QColor panelFooterSeed = m_theme.isDark
            ? blendColors(
                blendColors(m_theme.panelFooterBackground, m_theme.panelCardBackground, 0.46),
                m_theme.shellBackground,
                0.22
            )
            : blendColors(m_theme.panelFooterBackground, m_theme.panelCardBackground, 0.18);
        const QColor panelFooterSurface = withAlpha(panelFooterSeed, m_theme.isDark ? 184 : 214);
        const QColor panelFooterLine = withAlpha(
            blendColors(m_theme.panelFooterBorder, m_theme.shellBorder, m_theme.isDark ? 0.36 : 0.24),
            m_theme.isDark ? 198 : 184
        );
        const QColor glassHighlight = withAlpha(
            blendColors(m_theme.tabAccent, QColor(QStringLiteral("#ffffff")), m_theme.isDark ? 0.72 : 0.82),
            m_theme.isDark ? 148 : 188
        );
        const QColor glassRimTop = withAlpha(
            blendColors(m_theme.shellBorder, QColor(QStringLiteral("#ffffff")), m_theme.isDark ? 0.42 : 0.58),
            m_theme.isDark ? 174 : 206
        );
        const QColor glassRimBottom = withAlpha(
            blendColors(m_theme.shellBorder, QColor(QStringLiteral("#0f1724")), m_theme.isDark ? 0.40 : 0.24),
            m_theme.isDark ? 168 : 140
        );
        const QColor tabSelectedShine = withAlpha(
            blendColors(tabSelectedSurface, QColor(QStringLiteral("#ffffff")), m_theme.isDark ? 0.30 : 0.46),
            m_theme.isDark ? 190 : 232
        );
        const QColor utilitySurfaceTop = withAlpha(
            blendColors(utilitySurface, QColor(QStringLiteral("#ffffff")), m_theme.isDark ? 0.26 : 0.44),
            m_theme.isDark ? 126 : 168
        );
        const QColor topTabGlassSurface = withAlpha(
            blendColors(utilitySurface, m_theme.panelCardBackground, m_theme.isDark ? 0.36 : 0.24),
            m_theme.isDark ? 154 : 198
        );
        const QColor topTabGlassBorder = withAlpha(
            blendColors(m_theme.quickBorder, m_theme.tabSelectedBorder, 0.34),
            m_theme.isDark ? 182 : 172
        );
        const QColor topTabGlassHighlight = withAlpha(
            blendColors(m_theme.tabAccent, QColor(QStringLiteral("#ffffff")), m_theme.isDark ? 0.70 : 0.82),
            m_theme.isDark ? 110 : 156
        );
        const QColor panelTitleSurface = withAlpha(
            blendColors(panelFooterSurface, m_theme.tabSelectedBackground, m_theme.isDark ? 0.30 : 0.20),
            m_theme.isDark ? 166 : 206
        );
        const QColor panelTitleBorder = withAlpha(
            blendColors(panelFooterLine, m_theme.tabSelectedBorder, 0.28),
            m_theme.isDark ? 176 : 168
        );
        const QColor panelTitleShine = withAlpha(
            blendColors(panelTitleSurface, QColor(QStringLiteral("#ffffff")), m_theme.isDark ? 0.56 : 0.74),
            m_theme.isDark ? 88 : 136
        );
        const QColor panelTitleText = forceTextColor
            ? primaryText
            : ensureReadableTextColor(
                panelFooterSurface,
                preferredTitleText,
                6.0,
                QColor(QStringLiteral("#0f1823")),
                QColor(QStringLiteral("#f6f8fc"))
            );
        const QColor disabledSeed = blendColors(
            subtleText,
            m_theme.shellBackground,
            m_theme.isDark ? 0.18 : 0.30
        );
        const QColor disabledText = forceTextColor
            ? primaryText
            : ensureReadableTextColor(headerSurface, disabledSeed, 4.2);
        const QColor focusBorder = withAlpha(m_theme.buttonFocusBorder, 232);
        const QColor menuSurface = withAlpha(
            blendColors(m_theme.panelCardBackground, m_theme.panelFooterBackground, 0.22),
            m_theme.isDark ? 188 : 214
        );
        const QColor menuHoverSurface = withAlpha(
            blendColors(m_theme.panelFooterBackground, m_theme.panelCardBackground, 0.38),
            m_theme.isDark ? 196 : 218
        );
        const QColor menuBorder = withAlpha(
            blendColors(m_theme.panelCardBorder, m_theme.shellBorder, 0.20),
            192
        );
        const QColor fieldSurface = withAlpha(
            blendColors(m_theme.panelFooterBackground, m_theme.panelCardBackground, 0.16),
            m_theme.isDark ? 168 : 198
        );
        const QColor tooltipSurface = withAlpha(
            blendColors(m_theme.panelBodyAccent, m_theme.shellBackground, m_theme.isDark ? 0.32 : 0.16),
            m_theme.isDark ? 240 : 248
        );
        const QColor fieldText = forceTextColor
            ? primaryText
            : ensureReadableTextColor(fieldSurface, primaryText, 4.5);
        const QColor menuText = forceTextColor
            ? primaryText
            : ensureReadableTextColor(menuSurface, primaryText, 4.5);
        const QColor tooltipText = forceTextColor
            ? primaryText
            : ensureReadableTextColor(tooltipSurface, primaryText, 4.5);

        auto applyTextPalette = [primaryText, disabledText](QPalette& palette) {
            const QPalette::ColorGroup activeGroups[] = {
                QPalette::Active,
                QPalette::Inactive,
            };
            for (QPalette::ColorGroup group : activeGroups) {
                palette.setColor(group, QPalette::WindowText, primaryText);
                palette.setColor(group, QPalette::Text, primaryText);
                palette.setColor(group, QPalette::ButtonText, primaryText);
                palette.setColor(group, QPalette::ToolTipText, primaryText);
                palette.setColor(group, QPalette::HighlightedText, primaryText);
            }
            palette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
            palette.setColor(QPalette::Disabled, QPalette::Text, disabledText);
            palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
            palette.setColor(QPalette::Disabled, QPalette::ToolTipText, disabledText);
            palette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledText);
        };

        QPalette shellPalette = palette();
        applyTextPalette(shellPalette);
        setPalette(shellPalette);

        auto applyIndexedQss = [](QString style, const std::initializer_list<QString>& values) {
            int index = static_cast<int>(values.size());
            for (auto it = values.end(); it != values.begin();) {
                --it;
                style.replace(QStringLiteral("%") + QString::number(index), *it);
                --index;
            }
            return style;
        };

        setStyleSheet(applyIndexedQss(QStringLiteral(R"(
#FreeCADCommandTabNativeShell {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %35,stop:0.16 %41,stop:0.46 %22,stop:1 %1);
    border-bottom: 0px solid transparent;
    border-top: 0px solid %2;
}
#FreeCADCommandTabNativeShell,
#FreeCADCommandTabNativeShell * {
    color: %8;
}
#FreeCADCommandTabNativeShell QLabel {
    border: none;
    background: transparent;
}
#CommandTabHeader {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %43,stop:0.26 %44,stop:0.72 %41,stop:1 %22);
    border: 1px solid %42;
    border-top: 1px solid %37;
    border-bottom: 1px solid %38;
    border-radius: 12px;
    min-height: 42px;
}
QWidget#CommandTabPanelCard {
    background: transparent;
    border: none;
    border-radius: 0px;
}
QLabel#CommandTabBrandBadge {
    background: %33;
    border: 1px solid %34;
    border-radius: 10px;
    padding: 0px;
}
QWidget#CommandTabPanelBody {
    background: transparent;
    border: none;
    border-radius: 10px;
}
QWidget#CommandTabPanelFooter {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %44,stop:0.58 %6,stop:1 %22);
    border: 1px solid %45;
    border-top: 1px solid %37;
    border-radius: 12px;
}
QLabel#CommandTabPanelTitle {
    color: %7;
    font-weight: 700;
    background: transparent;
    border: none;
    padding: 0px 10px;
    margin: 0px;
    border-radius: 0px;
}
QToolButton[commandtabRole="panelOption"] {
    color: %8;
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %35,stop:0.40 %39,stop:1 %12);
    border: 1px solid %11;
    border-top: 1px solid %37;
    border-bottom: 1px solid %38;
    border-radius: 8px;
    padding: 0px 4px;
    font-weight: 700;
}
QToolButton[commandtabRole="panelOption"]:hover {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %43,stop:0.34 %40,stop:1 %20);
    border: 1px solid %11;
}
QToolButton[commandtabRole="panelOption"]:focus {
    border: 1px solid %23;
}
QToolButton[commandtabRole="panelExpand"] {
    color: %8;
    background: transparent;
    border: 1px solid transparent;
    border-radius: 5px;
    padding: 0px 3px;
    font-weight: 700;
}
QToolButton[commandtabRole="panelExpand"]:hover {
    background: %10;
    border: 1px solid %11;
}
QToolButton[commandtabRole="panelExpand"]:focus {
    border: 1px solid %23;
}
QToolButton[commandtabRole="panelPopupTool"] {
    color: %8;
    background: transparent;
    border: 1px solid transparent;
    border-radius: 6px;
    padding: 2px 3px;
}
QToolButton[commandtabRole="panelPopupTool"]:hover {
    background: %10;
    border: 1px solid %11;
}
QToolButton[commandtabRole="panelPopupTool"]:focus {
    border: 1px solid %23;
}
QToolButton[commandtabRole="panelPopupTool"]:disabled {
    color: %24;
}
QToolButton[commandtabRole="panelSideTool"] {
    color: %8;
    background: transparent;
    border: 1px solid transparent;
    border-radius: 6px;
    padding: 1px;
}
QToolButton[commandtabRole="panelSideTool"]:hover {
    background: %10;
    border: 1px solid %11;
}
QToolButton[commandtabRole="panelSideTool"]:focus {
    border: 1px solid %23;
}
QToolButton[commandtabRole="panelSideTool"]:disabled {
    color: %24;
}
QWidget#CommandTabQuickAccessBar {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %35,stop:0.24 %39,stop:0.72 %41,stop:1 %12);
    border: 1px solid %11;
    border-top: 1px solid %37;
    border-bottom: 1px solid %38;
    border-radius: 11px;
}
QWidget#CommandTabUtilityBar {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %35,stop:0.24 %39,stop:0.72 %41,stop:1 %12);
    border: 1px solid %11;
    border-top: 1px solid %37;
    border-bottom: 1px solid %38;
    border-radius: 11px;
}
QToolButton[commandtabRole="quick"] {
    color: %8;
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %39,stop:1 %12);
    border: 1px solid transparent;
    border-radius: 9px;
    padding: 1px;
}
QToolButton[commandtabRole="quick"]:hover {
    background: %10;
    border: 1px solid %13;
}
QToolButton[commandtabRole="quick"]:focus {
    border: 1px solid %23;
}
QToolButton[commandtabRole="quick"]:disabled {
    color: %24;
}
QToolButton[commandtabRole="utility"] {
    color: %8;
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %39,stop:1 %12);
    border: 1px solid %11;
    border-radius: 9px;
    padding: 1px 9px;
    font-weight: 600;
}
QToolButton[commandtabRole="utility"]:hover {
    background: %10;
    border: 1px solid %14;
}
QToolButton[commandtabRole="utility"]:focus {
    border: 1px solid %23;
}
QToolButton[commandtabRole="utility"]:disabled {
    color: %24;
}
QTabBar#CommandTabTabBar {
    background: transparent;
    border: none;
    border-radius: 0px;
    padding: 0px;
    margin: 0px;
}
QTabBar#CommandTabTabBar::scroller {
    width: 64px;
}
QTabBar#CommandTabTabBar::tab {
    color: %15;
    background: transparent;
    padding: 5px 12px 6px 11px;
    min-width: 46px;
    min-height: 24px;
    margin-right: 3px;
    border: 1px solid transparent;
    border-radius: 9px;
    font-weight: 600;
}
QTabBar#CommandTabTabBar[commandtabDensity="1"]::tab {
    padding: 4px 10px 5px 9px;
    min-width: 42px;
    margin-right: 2px;
}
QTabBar#CommandTabTabBar[commandtabDensity="2"]::tab {
    padding: 3px 8px 4px 7px;
    min-width: 38px;
    margin-right: 1px;
}
QTabBar#CommandTabTabBar::tab:selected {
    color: %16;
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %43,stop:0.34 %40,stop:0.70 %41,stop:1 %17);
    border: 1px solid %42;
    border-top: 1px solid %37;
    border-bottom: 1px solid %38;
    font-weight: 700;
}
QTabBar#CommandTabTabBar::tab:hover:!selected {
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %35,stop:1 %20);
    border: 1px solid %13;
}
QTabBar#CommandTabTabBar::tab:disabled {
    color: %24;
}
QTabBar#CommandTabTabBar QToolButton {
    color: %9;
    background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 %39,stop:1 %12);
    border: 1px solid transparent;
    border-radius: 0px;
    padding: 1px;
}
QTabBar#CommandTabTabBar QToolButton:hover {
    background: %10;
    border: 1px solid %11;
}
QTabBar#CommandTabTabBar QToolButton:disabled {
    color: %24;
}
QMenu {
    background: %25;
    border: 1px solid %26;
    border-radius: 10px;
    padding: 6px;
}
QMenu::item {
    color: %30;
    background: transparent;
    padding: 6px 10px;
    margin: 1px 3px;
    border: 1px solid transparent;
    border-radius: 7px;
}
QMenu::item:selected {
    background: %27;
    border: 1px solid %23;
}
QMenu::item:disabled {
    color: %24;
}
QMenu::separator {
    height: 6px;
    margin: 0px;
    background: transparent;
}
QWidget#CommandTabPanelPopupGrid {
    background: %1;
    border: none;
    border-radius: 8px;
}
QLineEdit {
    background: %28;
    color: %32;
    border: 1px solid %26;
    border-radius: 7px;
    padding: 4px 8px;
}
QLineEdit:focus {
    border: 1px solid %23;
}
QLineEdit:disabled {
    color: %24;
}
QToolTip {
    background: %29;
    color: %31;
    border: 1px solid %23;
    border-radius: 6px;
    padding: 4px 6px;
}
QScrollArea {
    border: none;
    background: transparent;
}
QScrollArea#CommandTabWorkbenchScrollArea {
    border: none;
    background: transparent;
    border-radius: 12px;
}
QWidget#CommandTabWorkbenchViewport {
    background: transparent;
    border-radius: 12px;
}
        )"),
            {
                m_theme.shellBackground.name(),
                m_theme.shellBorder.name(),
                headerTop.name(QColor::HexArgb),
                headerLine.name(QColor::HexArgb),
                cardBorder.name(QColor::HexArgb),
                panelFooterSurface.name(QColor::HexArgb),
                panelTitleText.name(QColor::HexArgb),
                primaryText.name(QColor::HexArgb),
                subtleText.name(),
                hoverSurface.name(QColor::HexArgb),
                utilityBorder.name(QColor::HexArgb),
                utilitySurface.name(QColor::HexArgb),
                dividerColor.name(QColor::HexArgb),
                m_theme.buttonActiveBorder.name(),
                tabText.name(),
                tabSelectedText.name(),
                tabSelectedSurface.name(QColor::HexArgb),
                tabSelectedBorder.name(QColor::HexArgb),
                tabSelectedBorder.name(QColor::HexArgb),
                tabHoverSurface.name(QColor::HexArgb),
                panelFooterLine.name(QColor::HexArgb),
                headerBottom.name(QColor::HexArgb),
                focusBorder.name(QColor::HexArgb),
                disabledText.name(QColor::HexArgb),
                menuSurface.name(QColor::HexArgb),
                menuBorder.name(QColor::HexArgb),
                menuHoverSurface.name(QColor::HexArgb),
                fieldSurface.name(QColor::HexArgb),
                tooltipSurface.name(QColor::HexArgb),
                menuText.name(QColor::HexArgb),
                tooltipText.name(QColor::HexArgb),
                fieldText.name(QColor::HexArgb),
                brandSurface.name(QColor::HexArgb),
                brandBorder.name(QColor::HexArgb),
                glassHighlight.name(QColor::HexArgb),
                headerBottom.name(QColor::HexArgb),
                glassRimTop.name(QColor::HexArgb),
                glassRimBottom.name(QColor::HexArgb),
                utilitySurfaceTop.name(QColor::HexArgb),
                tabSelectedShine.name(QColor::HexArgb),
                topTabGlassSurface.name(QColor::HexArgb),
                topTabGlassBorder.name(QColor::HexArgb),
                topTabGlassHighlight.name(QColor::HexArgb),
                panelTitleSurface.name(QColor::HexArgb),
                panelTitleBorder.name(QColor::HexArgb),
                panelTitleShine.name(QColor::HexArgb),
            }));

        m_tabTextColor = tabText;
        m_tabSelectedTextColor = tabSelectedText;
        m_tabDisabledTextColor = disabledText;

        if (m_tabBar != nullptr) {
            QPalette tabPalette = m_tabBar->palette();
            tabPalette.setColor(QPalette::Active, QPalette::WindowText, tabText);
            tabPalette.setColor(QPalette::Inactive, QPalette::WindowText, tabText);
            tabPalette.setColor(QPalette::Active, QPalette::Text, tabText);
            tabPalette.setColor(QPalette::Inactive, QPalette::Text, tabText);
            tabPalette.setColor(QPalette::Active, QPalette::ButtonText, tabText);
            tabPalette.setColor(QPalette::Inactive, QPalette::ButtonText, tabText);
            tabPalette.setColor(QPalette::Disabled, QPalette::WindowText, m_tabDisabledTextColor);
            tabPalette.setColor(QPalette::Disabled, QPalette::Text, m_tabDisabledTextColor);
            tabPalette.setColor(QPalette::Disabled, QPalette::ButtonText, m_tabDisabledTextColor);
            m_tabBar->setPalette(tabPalette);
            applyTabTextColors();
            m_tabBar->update();
        }
    }

    bool ensureWorkbenchPage(int index, int initialPanelBudget = 6, bool deferInitialChunk = false)
    {
        if (index < 0 || index >= m_workbenchEntries.size() || index >= m_workbenchPagesBuilt.size()) {
            return false;
        }
        if (index < m_workbenchPagesDirty.size() && m_workbenchPagesDirty.at(index)) {
            resetWorkbenchPage(index);
            m_workbenchPagesDirty[index] = false;
        }
        if (m_workbenchPagesBuilt.at(index)) {
            return true;
        }

        auto& pageState = m_pageBuildStates[index];
        if (pageState.scrollArea == nullptr) {
            auto* previousWidget = m_stack->widget(index);
            pageState = createWorkbenchPageState();
            m_stack->insertWidget(index, pageState.scrollArea);
            if (previousWidget != nullptr) {
                m_stack->removeWidget(previousWidget);
                previousWidget->deleteLater();
            }
        }
        if (deferInitialChunk) {
            const int clampedBudget = std::max(1, initialPanelBudget);
            QTimer::singleShot(0, this, [this, index, clampedBudget]() {
                buildWorkbenchPageChunk(index, clampedBudget);
            });
        } else {
            buildWorkbenchPageChunk(index, std::max(1, initialPanelBudget));
        }
        return true;
    }

    void refreshWorkbenchPageLayout(int index)
    {
        if (index < 0 || index >= m_pageBuildStates.size()) {
            return;
        }

        auto& pageState = m_pageBuildStates[index];
        if (pageState.container == nullptr || pageState.layout == nullptr) {
            return;
        }

        for (int itemIndex = 0; itemIndex < pageState.layout->count(); ++itemIndex) {
            QLayoutItem* item = pageState.layout->itemAt(itemIndex);
            QWidget* panelWidget = item != nullptr ? item->widget() : nullptr;
            if (panelWidget == nullptr || panelWidget->objectName() != QStringLiteral("CommandTabPanelCard")) {
                continue;
            }
            refreshPanelWidgetGeometry(panelWidget);
        }

        pageState.container->updateGeometry();
        if (pageState.layout != nullptr) {
            pageState.layout->invalidate();
            pageState.layout->activate();
        }
        pageState.container->adjustSize();
        if (pageState.scrollArea != nullptr) {
            pageState.scrollArea->updateGeometry();
            pageState.scrollArea->viewport()->update();
        }
    }

    struct PageBuildState
    {
        QScrollArea* scrollArea = nullptr;
        QWidget* container = nullptr;
        QHBoxLayout* layout = nullptr;
        int nextPanelIndex = 0;
        bool chunkScheduled = false;
        bool stretchAdded = false;
    };

    QMargins panelContainerMargins() const
    {
        const int density = std::clamp(m_adaptiveDensityLevel, 0, 2);
        const int horizontalReduce = density >= 2 ? 1 : 0;
        const int verticalReduce = density >= 1 ? 1 : 0;
        if (m_settingsState.compactPanelLayout) {
            return QMargins(
                scaledPx(std::max(1, 2 - horizontalReduce)),
                scaledPx(std::max(2, 4 - verticalReduce)),
                scaledPx(std::max(1, 2 - horizontalReduce)),
                scaledPx(std::max(2, 4 - verticalReduce))
            );
        }
        return QMargins(
            scaledPx(std::max(1, 3 - horizontalReduce)),
            scaledPx(std::max(2, 4 - verticalReduce)),
            scaledPx(std::max(1, 3 - horizontalReduce)),
            scaledPx(std::max(2, 4 - verticalReduce))
        );
    }

    int panelContainerSpacing() const
    {
        const int density = std::clamp(m_adaptiveDensityLevel, 0, 2);
        if (!m_settingsState.compactPanelLayout) {
            return scaledPx(std::max(2, 6 - (density >= 2 ? 2 : (density == 1 ? 1 : 0))));
        }
        const int spacing = std::clamp(
            scaledPx(std::clamp(m_settingsState.compactPanelSpacing + 1, 1, 7)),
            1,
            scaledPx(10)
        );
        return std::max(1, spacing - (density >= 2 ? 1 : 0));
    }

    PageBuildState createWorkbenchPageState()
    {
        PageBuildState state;
        state.scrollArea = new QScrollArea(this);
        state.scrollArea->setObjectName(QStringLiteral("CommandTabWorkbenchScrollArea"));
        state.scrollArea->setFrameShape(QFrame::NoFrame);
        state.scrollArea->setWidgetResizable(true);
        state.scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        state.scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        state.scrollArea->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        state.scrollArea->setAttribute(Qt::WA_StyledBackground, true);
        if (QWidget* viewport = state.scrollArea->viewport()) {
            viewport->setObjectName(QStringLiteral("CommandTabWorkbenchViewport"));
            viewport->setAttribute(Qt::WA_StyledBackground, true);
            installRoundedMask(viewport, scaledPx(10));
        }
        if (auto* scrollBar = state.scrollArea->horizontalScrollBar()) {
            scrollBar->setSingleStep(scaledPx(24));
            scrollBar->setPageStep(scaledPx(120));
            scrollBar->setStyleSheet(QString());
            if (QStyle* style = nativeScrollBarStyle()) {
                scrollBar->setStyle(style);
            }
        }

        state.container = new QWidget(state.scrollArea);
        state.layout = new QHBoxLayout(state.container);
        state.layout->setContentsMargins(panelContainerMargins());
        state.layout->setSpacing(panelContainerSpacing());
        state.layout->setSizeConstraint(QLayout::SetMinimumSize);
        state.container->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        state.scrollArea->setWidget(state.container);
        return state;
    }

    void resetWorkbenchPage(int index)
    {
        if (index < 0 || index >= m_stack->count() || index >= m_pageBuildStates.size()) {
            return;
        }

        auto& pageState = m_pageBuildStates[index];
        pageState = PageBuildState();
        m_workbenchPagesBuilt[index] = false;

        auto* previousWidget = m_stack->widget(index);
        auto* placeholder = new QWidget(m_stack);
        placeholder->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        m_stack->insertWidget(index, placeholder);
        if (previousWidget != nullptr) {
            m_stack->removeWidget(previousWidget);
            previousWidget->deleteLater();
        }
    }

    struct PanelMetrics
    {
        int smallCount = 0;
        int mediumCount = 0;
        int largeCount = 0;
        int iconOnlyCount = 0;
        int textCount = 0;
    };

    PanelMetrics collectPanelMetrics(const CommandTabPanelBodyWidget* bodyFrame) const
    {
        PanelMetrics metrics;
        if (bodyFrame == nullptr) {
            return metrics;
        }

        for (QObject* child : bodyFrame->children()) {
            auto* childWidget = qobject_cast<QWidget*>(child);
            auto* button = dynamic_cast<CommandTabCommandButton*>(childWidget);
            if (button == nullptr) {
                continue;
            }
            if (button->showsText()) {
                ++metrics.textCount;
                if (button->isLargeButton()) {
                    ++metrics.largeCount;
                } else if (button->isMediumButton()) {
                    ++metrics.mediumCount;
                } else {
                    ++metrics.smallCount;
                }
            } else {
                ++metrics.iconOnlyCount;
                // Icon-only commands are treated as compact square buttons at
                // layout level, so use the small footprint for panel sizing.
                ++metrics.smallCount;
            }
        }
        return metrics;
    }

    int compactPanelWidthForBody(int rawWidth, const PanelMetrics& metrics) const
    {
        if (!m_settingsState.compactPanelLayout) {
            return rawWidth;
        }

        int minimumWidth = scaledPx(70);
        if (metrics.textCount == 0 && metrics.iconOnlyCount > 0) {
            minimumWidth = scaledPx(72);
        } else if (metrics.largeCount > 0) {
            minimumWidth = scaledPx(118);
        } else if (metrics.mediumCount > 0) {
            minimumWidth = scaledPx(96);
        } else if (metrics.smallCount > 0) {
            minimumWidth = scaledPx(72);
        }

        // The compact panel width helper keeps panels from expanding below
        // a readable baseline while letting body layout drive the actual width.
        return std::max(rawWidth, minimumWidth);
    }

    void refreshPanelWidgetGeometry(QWidget* panelWidget)
    {
        if (panelWidget == nullptr) {
            return;
        }

        CommandTabPanelBodyWidget* bodyFrame = nullptr;
        QWidget* footerWidget = nullptr;
        for (QObject* child : panelWidget->children()) {
            auto* childWidget = qobject_cast<QWidget*>(child);
            if (childWidget == nullptr) {
                continue;
            }
            if (bodyFrame == nullptr) {
                bodyFrame = dynamic_cast<CommandTabPanelBodyWidget*>(childWidget);
                if (bodyFrame != nullptr) {
                    continue;
                }
            }
            if (footerWidget == nullptr && childWidget->objectName() == QStringLiteral("CommandTabPanelFooter")) {
                footerWidget = childWidget;
            }
        }

        if (bodyFrame == nullptr) {
            return;
        }

        bodyFrame->applySettings(m_settingsState);
        const PanelMetrics panelMetrics = collectPanelMetrics(bodyFrame);
        int panelWidth = compactPanelWidthForBody(bodyFrame->sizeHint().width(), panelMetrics);
        const int panelInset = scaledPx(2);
        const int panelContentSpacing = scaledPx(2);
        int panelInnerWidth = std::max(
            bodyFrame->sizeHint().width(),
            std::max(scaledPx(42), panelWidth - (panelInset * 2))
        );
        // Body keeps its natural content width; panel card and footer are resized to
        // panelWidth. The QVBoxLayout AlignHCenter alignment centers the body inside.

        int footerHeight = 0;
        if (footerWidget != nullptr) {
            if (auto* footerLayout = qobject_cast<QHBoxLayout*>(footerWidget->layout())) {
                if (m_settingsState.compactPanelLayout) {
                    footerLayout->setContentsMargins(scaledPx(6), scaledPx(3), scaledPx(6), scaledPx(3));
                    footerLayout->setSpacing(scaledPx(2));
                } else {
                    footerLayout->setContentsMargins(scaledPx(9), scaledPx(3), scaledPx(9), scaledPx(3));
                    footerLayout->setSpacing(scaledPx(3));
                }
            }
            footerWidget->setFixedWidth(panelInnerWidth);
            bodyFrame->setFixedWidth(panelInnerWidth);

            if (auto* titleLabel = footerWidget->findChild<QLabel*>(QStringLiteral("CommandTabPanelTitle"))) {
                QFont titleFont = titleLabel->font();
                titleFont.setPointSize(std::clamp(scaledPx(10), 7, 20));
                titleFont.setWeight(QFont::Bold);
                titleLabel->setFont(titleFont);
                const QString panelTitle = titleLabel->toolTip();
                const int desiredTitleWidth = titleLabel->fontMetrics().horizontalAdvance(panelTitle) + scaledPx(18);
                panelInnerWidth = std::max(panelInnerWidth, desiredTitleWidth + scaledPx(10));
                footerWidget->setFixedWidth(panelInnerWidth);
                bodyFrame->setFixedWidth(panelInnerWidth);
                const int titleHorizontalPadding = m_settingsState.compactPanelLayout
                    ? scaledPx(8)
                    : scaledPx(10);
                const int titleWidth = std::max(
                    m_settingsState.compactPanelLayout ? scaledPx(68) : scaledPx(82),
                    panelInnerWidth - titleHorizontalPadding
                );
                const int nextFooterHeight = commandtabPanelFooterHeight(titleLabel->fontMetrics());
                const int titleHeight = std::max(scaledPx(18), nextFooterHeight - scaledPx(8));
                titleLabel->setFixedWidth(titleWidth);
                titleLabel->setFixedHeight(titleHeight);
                titleLabel->setText(panelTitle);
                footerWidget->setFixedHeight(nextFooterHeight);

                const QRect safeRect = footerWidget->rect().adjusted(
                    scaledPx(2),
                    scaledPx(2),
                    -scaledPx(2),
                    -scaledPx(2)
                );
                const int safeTitleWidth = std::max(
                    scaledPx(42),
                    std::min(titleLabel->width(), safeRect.width() - scaledPx(2))
                );
                const int safeTitleHeight = std::max(
                    scaledPx(16),
                    std::min(titleLabel->height(), safeRect.height() - scaledPx(4))
                );
                titleLabel->setFixedWidth(safeTitleWidth);
                titleLabel->setFixedHeight(safeTitleHeight);
            }

            footerHeight = footerWidget->minimumHeight();
            footerWidget->updateGeometry();
            footerWidget->update();
        }

        panelWidth = panelInnerWidth + (panelInset * 2);
        panelWidget->setFixedWidth(panelWidth);
        panelWidget->setFixedHeight(bodyFrame->sizeHint().height() + footerHeight + (panelInset * 2) + panelContentSpacing);
        panelWidget->updateGeometry();
        panelWidget->update();
    }

    void scheduleWorkbenchPageChunk(int index)
    {
        if (index < 0 || index >= m_pageBuildStates.size()) {
            return;
        }

        auto& pageState = m_pageBuildStates[index];
        if (pageState.chunkScheduled) {
            return;
        }

        pageState.chunkScheduled = true;
        const int delayMs =
            (m_stack != nullptr && m_stack->currentIndex() == index) ? 0 : 22;
        QTimer::singleShot(delayMs, this, [this, index]() {
            if (index < 0 || index >= m_pageBuildStates.size()) {
                return;
            }
            m_pageBuildStates[index].chunkScheduled = false;
            buildWorkbenchPageChunk(index, 3);
        });
    }

    void buildWorkbenchPageChunk(int index, int panelBudget)
    {
        if (index < 0 || index >= m_workbenchEntries.size() || index >= m_pageBuildStates.size()) {
            return;
        }

        auto& pageState = m_pageBuildStates[index];
        if (pageState.layout == nullptr) {
            return;
        }

        const auto& workbench = m_workbenchEntries.at(index);
        const int totalPanels = static_cast<int>(workbench.panels.size());
        int builtCount = 0;
        while (pageState.nextPanelIndex < totalPanels && builtCount < panelBudget) {
            pageState.layout->addWidget(createPanelWidget(workbench.id, workbench.panels.at(pageState.nextPanelIndex)));
            ++pageState.nextPanelIndex;
            ++builtCount;
        }

        if (pageState.nextPanelIndex >= totalPanels) {
            if (!pageState.stretchAdded) {
                pageState.layout->addStretch(1);
                pageState.stretchAdded = true;
            }
            m_workbenchPagesBuilt[index] = true;
        } else {
            scheduleWorkbenchPageChunk(index);
        }

        const bool finished = pageState.nextPanelIndex >= totalPanels;
        if (pageState.container != nullptr) {
            pageState.container->updateGeometry();
            if (finished) {
                pageState.container->adjustSize();
            }
        }
        if (pageState.scrollArea != nullptr) {
            pageState.scrollArea->updateGeometry();
            pageState.scrollArea->viewport()->update();
        }
        const bool shouldNotify =
            finished || (m_stack != nullptr && m_stack->currentIndex() == index);
        if (shouldNotify) {
            updateGeometry();
            if (m_contentChangedHandler) {
                m_contentChangedHandler();
            }
        }
        if (finished) {
            scheduleBackgroundPageWarmup();
        }
    }

    void scheduleBackgroundPageWarmup()
    {
        if (m_backgroundPageWarmupScheduled || m_workbenchEntries.size() <= 1) {
            return;
        }
        m_backgroundPageWarmupScheduled = true;
        QTimer::singleShot(240, this, [this]() {
            m_backgroundPageWarmupScheduled = false;
            continueBackgroundPageWarmup();
        });
    }

    void continueBackgroundPageWarmup()
    {
        if (m_workbenchEntries.size() <= 1) {
            return;
        }
        const int currentIndex = (m_stack != nullptr) ? m_stack->currentIndex() : -1;
        const int count = static_cast<int>(m_workbenchEntries.size());
        if (count <= 0) {
            return;
        }

        for (int attempt = 0; attempt < count; ++attempt) {
            const int candidateIndex = (m_backgroundWarmupCursor + attempt) % count;
            if (candidateIndex == currentIndex) {
                continue;
            }
            if (candidateIndex < 0 || candidateIndex >= m_workbenchPagesBuilt.size()) {
                continue;
            }
            if (m_workbenchPagesBuilt.at(candidateIndex)) {
                continue;
            }
            if (candidateIndex < m_pageBuildStates.size()
                && m_pageBuildStates[candidateIndex].chunkScheduled) {
                continue;
            }

            m_backgroundWarmupCursor = candidateIndex + 1;
            ensureWorkbenchPage(candidateIndex, 2, true);
            scheduleBackgroundPageWarmup();
            return;
        }
    }

    QString panelUsageKey(const QString& workbenchId, const QString& panelId) const
    {
        const QString normalizedWorkbenchId = workbenchId.trimmed();
        const QString normalizedPanelId = panelId.trimmed();
        if (normalizedWorkbenchId.isEmpty() || normalizedPanelId.isEmpty()) {
            return QString();
        }
        return normalizedWorkbenchId + QStringLiteral("::") + normalizedPanelId;
    }

    const CommandTabPanelEntry* panelEntryByIds(const QString& workbenchId, const QString& panelId) const
    {
        if (workbenchId.isEmpty() || panelId.isEmpty()) {
            return nullptr;
        }
        for (const auto& workbench : m_workbenchEntries) {
            if (workbench.id != workbenchId) {
                continue;
            }
            for (const auto& panel : workbench.panels) {
                if (panel.id == panelId) {
                    return &panel;
                }
            }
            break;
        }
        return nullptr;
    }

    QVector<CommandTabCommandEntry> collectPanelActionableCommands(
        const CommandTabPanelEntry& panel,
        bool includeDropdownChildren = true
    ) const
    {
        QVector<CommandTabCommandEntry> commands;
        QSet<QString> seenIds;
        for (const auto& command : panel.commands) {
            if (command.type == QStringLiteral("separator")) {
                continue;
            }
            if (!command.id.isEmpty() && !seenIds.contains(command.id)) {
                seenIds.insert(command.id);
                commands.push_back(command);
            }
            if (!includeDropdownChildren || command.menuCommands.isEmpty()) {
                continue;
            }
            for (const auto& menuCommand : command.menuCommands) {
                if (
                    menuCommand.type == QStringLiteral("separator")
                    || menuCommand.id.isEmpty()
                    || seenIds.contains(menuCommand.id)
                ) {
                    continue;
                }
                seenIds.insert(menuCommand.id);
                commands.push_back(menuCommand);
            }
        }
        return commands;
    }

    CommandTabCommandEntry findActionableCommandById(
        const QVector<CommandTabCommandEntry>& commands,
        const QString& commandId
    ) const
    {
        if (commandId.isEmpty()) {
            return CommandTabCommandEntry();
        }
        for (const auto& command : commands) {
            if (command.id == commandId) {
                return command;
            }
        }
        return CommandTabCommandEntry();
    }

    CommandTabCommandEntry resolvePrimaryPanelCommand(const QString& workbenchId, const CommandTabPanelEntry& panel) const
    {
        const QVector<CommandTabCommandEntry> directCommands = collectPanelActionableCommands(panel, false);
        const QVector<CommandTabCommandEntry> allCommands = collectPanelActionableCommands(panel, true);
        const CommandTabCommandEntry firstCommand = !directCommands.isEmpty()
            ? directCommands.first()
            : (!allCommands.isEmpty() ? allCommands.first() : CommandTabCommandEntry());
        if (firstCommand.id.isEmpty()) {
            return CommandTabCommandEntry();
        }

        if (!m_settingsState.panelDropdownPrimaryRecent) {
            return firstCommand;
        }

        const QString usageKey = panelUsageKey(workbenchId, panel.id);
        const QStringList recentHistory = m_recentPanelCommandHistory.value(usageKey);
        for (const auto& recentCommandId : recentHistory) {
            const CommandTabCommandEntry recentCommand = findActionableCommandById(
                allCommands,
                recentCommandId.trimmed()
            );
            if (!recentCommand.id.isEmpty()) {
                return recentCommand;
            }
        }
        return firstCommand;
    }

    QVector<CommandTabCommandEntry> resolveRecentSidePanelCommands(
        const QString& workbenchId,
        const CommandTabPanelEntry& panel,
        const QString& primaryCommandId
    ) const
    {
        QVector<CommandTabCommandEntry> commands;
        const int requestedCount = std::clamp(m_settingsState.panelDropdownRecentToolCount, 0, 9);
        if (requestedCount <= 0) {
            return commands;
        }

        const QVector<CommandTabCommandEntry> allCommands = collectPanelActionableCommands(panel, true);
        if (allCommands.isEmpty()) {
            return commands;
        }

        auto appendIfAllowed = [&commands, requestedCount, &primaryCommandId](const CommandTabCommandEntry& command) {
            if (
                command.id.isEmpty()
                || command.id == primaryCommandId
                || commands.size() >= requestedCount
            ) {
                return;
            }
            for (const auto& existing : commands) {
                if (existing.id == command.id) {
                    return;
                }
            }
            commands.push_back(command);
        };

        const QString usageKey = panelUsageKey(workbenchId, panel.id);
        const QStringList recentHistory = m_recentPanelCommandHistory.value(usageKey);
        for (const auto& recentCommandId : recentHistory) {
            const CommandTabCommandEntry recentCommand = findActionableCommandById(
                allCommands,
                recentCommandId.trimmed()
            );
            appendIfAllowed(recentCommand);
        }
        for (const auto& command : allCommands) {
            appendIfAllowed(command);
        }
        return commands;
    }

    void queuePanelPrimaryRefresh(const QString& panelKey)
    {
        if (
            panelKey.isEmpty()
            || !m_settingsState.panelDropdownModeEnabled
            || (
                !m_settingsState.panelDropdownPrimaryRecent
                && m_settingsState.panelDropdownRecentToolCount <= 0
            )
            || m_pendingPanelPrimaryRefresh.contains(panelKey)
        ) {
            return;
        }
        m_pendingPanelPrimaryRefresh.insert(panelKey);
        QTimer::singleShot(0, this, [this, panelKey]() {
            m_pendingPanelPrimaryRefresh.remove(panelKey);
            refreshPanelWidgetByUsageKey(panelKey);
        });
    }

    bool refreshPanelWidgetByUsageKey(const QString& panelKey)
    {
        if (panelKey.isEmpty()) {
            return false;
        }

        for (auto& pageState : m_pageBuildStates) {
            if (pageState.layout == nullptr) {
                continue;
            }
            for (int itemIndex = 0; itemIndex < pageState.layout->count(); ++itemIndex) {
                QLayoutItem* item = pageState.layout->itemAt(itemIndex);
                QWidget* panelWidget = item != nullptr ? item->widget() : nullptr;
                if (panelWidget == nullptr || panelWidget->objectName() != QStringLiteral("CommandTabPanelCard")) {
                    continue;
                }
                if (panelWidget->property("commandtabPanelUsageKey").toString() != panelKey) {
                    continue;
                }

                const QString workbenchId = panelWidget->property("commandtabWorkbenchId").toString();
                const QString panelId = panelWidget->property("commandtabPanelId").toString();
                const CommandTabPanelEntry* panelEntry = panelEntryByIds(workbenchId, panelId);
                if (panelEntry == nullptr) {
                    return false;
                }

                QWidget* replacement = createPanelWidget(workbenchId, *panelEntry);
                pageState.layout->insertWidget(itemIndex, replacement);
                pageState.layout->removeWidget(panelWidget);
                panelWidget->deleteLater();

                if (pageState.container != nullptr) {
                    pageState.container->updateGeometry();
                    pageState.container->adjustSize();
                }
                if (pageState.scrollArea != nullptr) {
                    pageState.scrollArea->updateGeometry();
                    pageState.scrollArea->viewport()->update();
                }
                updateGeometry();
                if (m_contentChangedHandler) {
                    m_contentChangedHandler();
                }
                return true;
            }
        }
        return false;
    }

    void updateCollapseButton()
    {
        if (m_collapseButton == nullptr) {
            return;
        }
        const bool manualToggleEnabled = !m_settingsState.ribbonAutoHide;
        const QString symbol = m_ribbonCollapsed
            ? QStringLiteral("\342\226\274")   // ▼ expand
            : QStringLiteral("\342\226\262");  // ▲ collapse
        m_collapseButton->setText(symbol);
        m_collapseButton->setEnabled(manualToggleEnabled);
        if (!manualToggleEnabled) {
            m_collapseButton->setToolTip(
                QCoreApplication::translate(
                    "CommandTabShellWidget",
                    "Manual ribbon toggle is disabled while auto-hide is active"
                )
            );
        } else {
            m_collapseButton->setToolTip(
                m_ribbonCollapsed
                    ? QCoreApplication::translate("CommandTabShellWidget", "Expand ribbon")
                    : QCoreApplication::translate("CommandTabShellWidget", "Collapse ribbon")
            );
        }
    }

    void setRibbonCollapsed(bool collapsed, bool animate = true)
    {
        if (m_ribbonCollapsed == collapsed) {
            return;
        }
        m_ribbonCollapsed = collapsed;
        updateCollapseButton();

        if (m_stack == nullptr) {
            return;
        }

        if (m_stackAnimation == nullptr) {
            m_stackAnimation = new QPropertyAnimation(m_stack, "maximumHeight", this);
            m_stackAnimation->setDuration(220);
            // Each animated frame drives the dock resize so it follows in real-time
            connect(m_stackAnimation, &QPropertyAnimation::valueChanged, this,
                [this](const QVariant&) {
                    if (m_contentChangedHandler) {
                        m_contentChangedHandler();
                    }
                });
        }
        m_stackAnimation->stop();
        disconnect(m_stackAnimation, &QPropertyAnimation::finished, nullptr, nullptr);

        if (collapsed) {
            const int startH = m_stack->height() > 0 ? m_stack->height() : m_stack->sizeHint().height();
            if (animate) {
                m_stackAnimation->setEasingCurve(QEasingCurve::InOutQuart);
                m_stackAnimation->setStartValue(startH);
                m_stackAnimation->setEndValue(0);
                connect(m_stackAnimation, &QPropertyAnimation::finished, this, [this]() {
                    m_stack->hide();
                    m_stack->setMaximumHeight(QWIDGETSIZE_MAX);
                    if (m_contentChangedHandler) {
                        m_contentChangedHandler();
                    }
                });
                m_stackAnimation->start();
            } else {
                m_stack->hide();
                if (m_contentChangedHandler) {
                    m_contentChangedHandler();
                }
            }
        } else {
            const int targetH = std::max(m_stack->sizeHint().height(), scaledPx(60));
            m_stack->setMaximumHeight(0);
            m_stack->show();
            if (animate) {
                m_stackAnimation->setEasingCurve(QEasingCurve::OutQuart);
                m_stackAnimation->setStartValue(0);
                m_stackAnimation->setEndValue(targetH);
                connect(m_stackAnimation, &QPropertyAnimation::finished, this, [this]() {
                    m_stack->setMaximumHeight(QWIDGETSIZE_MAX);
                    if (m_contentChangedHandler) {
                        m_contentChangedHandler();
                    }
                });
                m_stackAnimation->start();
            } else {
                m_stack->setMaximumHeight(QWIDGETSIZE_MAX);
                if (m_contentChangedHandler) {
                    m_contentChangedHandler();
                }
            }
        }
    }

    void startAutoHideCountdown()
    {
        if (!m_settingsState.ribbonAutoHide || m_ribbonCollapsed) {
            return;
        }
        if (m_autoHideTimer == nullptr) {
            m_autoHideTimer = new QTimer(this);
            m_autoHideTimer->setSingleShot(true);
            connect(m_autoHideTimer, &QTimer::timeout, this, [this]() {
                if (m_settingsState.ribbonAutoHide && !m_ribbonCollapsed && !underMouse()) {
                    setRibbonCollapsed(true, true);
                }
            });
        }
        m_autoHideTimer->start(std::max(300, m_settingsState.ribbonAutoHideDelayMs));
    }

    void cancelAutoHideCountdown()
    {
        if (m_autoHideTimer != nullptr) {
            m_autoHideTimer->stop();
        }
    }

    void dispatchCommand(const QString& commandId, const QString& panelKey = QString())
    {
        const QString normalizedCommandId = commandId.trimmed();
        if (normalizedCommandId.isEmpty()) {
            return;
        }
        if (!panelKey.isEmpty()) {
            QStringList history = m_recentPanelCommandHistory.value(panelKey);
            history.removeAll(normalizedCommandId);
            history.prepend(normalizedCommandId);
            while (history.size() > 24) {
                history.removeLast();
            }
            m_recentPanelCommandHistory.insert(panelKey, history);
            queuePanelPrimaryRefresh(panelKey);
        }
        if (m_commandHandler) {
            m_commandHandler(normalizedCommandId);
        }
        if (!normalizedCommandId.startsWith(QStringLiteral("__workbench__:"))) {
            startAutoHideCountdown();
        }
    }

    QToolButton* createPanelPopupToolButton(
        const CommandTabCommandEntry& command,
        const QString& panelKey,
        QMenu* ownerMenu,
        QWidget* parent
    )
    {
        if (ownerMenu == nullptr || parent == nullptr || command.id.isEmpty()) {
            return nullptr;
        }

        auto* button = new QToolButton(parent);
        button->setAutoRaise(false);
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        button->setProperty("commandtabRole", QStringLiteral("panelPopupTool"));
        button->setProperty("commandtabCommandId", command.id);
        button->setProperty("commandtabHasMenuCommands", !command.menuCommands.isEmpty());
        button->setProperty("commandtabMenuCommandCount", command.menuCommands.size());

        const int iconSize = std::clamp(
            scaledPx(std::clamp(m_settingsState.panelDropdownPopupIconSize, 12, 48)),
            scaledPx(10),
            scaledPx(64)
        );
        const bool showText = m_settingsState.panelDropdownPopupShowText;
        button->setIconSize(QSize(iconSize, iconSize));
        button->setToolButtonStyle(showText ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly);
        button->setProperty(
            "commandtabPreferredToolButtonStyle",
            showText
                ? static_cast<int>(Qt::ToolButtonTextUnderIcon)
                : static_cast<int>(Qt::ToolButtonIconOnly)
        );

        const QAction* sourceAction = resolveActionForCommandId(command.id);
        const QString label = commandHoverLabel(command, sourceAction);
        button->setText(showText ? label : QString());
        button->setToolTip(label);

        QIcon icon = loadCommandEntryIcon(command);
        if (sourceAction != nullptr && !sourceAction->icon().isNull()) {
            icon = sourceAction->icon();
        }
        if (!icon.isNull()) {
            button->setIcon(icon);
        } else {
            const QColor badgeBackground = blendColors(
                popupMenuSurfaceColor(),
                m_theme.panelBodyAccent,
                m_theme.isDark ? 0.46 : 0.34
            );
            const QColor badgeForeground = ensureReadableTextColor(
                badgeBackground,
                m_theme.buttonText,
                4.5
            );
            button->setIcon(
                buildMonogramIcon(label, QSize(iconSize, iconSize), badgeBackground, badgeForeground)
            );
        }

        if (showText) {
            const int textWidth = button->fontMetrics().horizontalAdvance(label);
            const int buttonWidth = std::clamp(
                std::max(iconSize + scaledPx(14), textWidth + scaledPx(18)),
                scaledPx(54),
                scaledPx(200)
            );
            const int buttonHeight = iconSize + button->fontMetrics().height() + scaledPx(16);
            button->setFixedSize(buttonWidth, buttonHeight);
        } else {
            const int edge = iconSize + scaledPx(12);
            button->setFixedSize(edge, edge);
        }

        applyPanelPopupToolButtonTheme(button);
        bindWidgetEnabledToCommand(button, command.id);
        connect(button, &QToolButton::clicked, this, [this, ownerMenu, panelKey, command]() {
            dispatchCommand(command.id, panelKey);
            ownerMenu->close();
        });
        return button;
    }

    QMenu* createPanelExpandMenu(
        const QString& panelKey,
        const CommandTabPanelEntry& panel,
        QWidget* parent
    )
    {
        const QVector<CommandTabCommandEntry> commands = collectPanelActionableCommands(panel, true);
        if (commands.isEmpty()) {
            return nullptr;
        }

        auto* menu = new QMenu(parent);
        menu->setAttribute(Qt::WA_StyledBackground, true);
        installRoundedMask(menu, scaledPx(10));
        applyPopupMenuTheme(menu);
        auto* contentWidget = new QWidget(menu);
        contentWidget->setObjectName(QStringLiteral("CommandTabPanelPopupGrid"));
        contentWidget->setAttribute(Qt::WA_StyledBackground, true);
        installRoundedMask(contentWidget, scaledPx(8));
        auto* gridLayout = new QGridLayout(contentWidget);
        gridLayout->setContentsMargins(
            scaledPx(6),
            scaledPx(6),
            scaledPx(6),
            scaledPx(6)
        );
        gridLayout->setHorizontalSpacing(scaledPx(4));
        gridLayout->setVerticalSpacing(scaledPx(4));

        const int configuredColumns = std::clamp(m_settingsState.panelDropdownPopupColumns, 1, 8);
        const int columns = std::max(1, std::min(configuredColumns, static_cast<int>(commands.size())));
        int visibleIndex = 0;
        for (const auto& command : commands) {
            auto* button = createPanelPopupToolButton(command, panelKey, menu, contentWidget);
            if (button == nullptr) {
                continue;
            }
            const int row = visibleIndex / columns;
            const int column = visibleIndex % columns;
            gridLayout->addWidget(button, row, column);
            ++visibleIndex;
        }

        if (visibleIndex <= 0) {
            menu->deleteLater();
            return nullptr;
        }

        auto* widgetAction = new QWidgetAction(menu);
        widgetAction->setDefaultWidget(contentWidget);
        menu->addAction(widgetAction);
        return menu;
    }

    void showWorkbenchTabPopup(int index)
    {
        if (m_tabBar == nullptr || index < 0 || index >= m_workbenchEntries.size()) {
            return;
        }
        const auto& workbench = m_workbenchEntries.at(index);
        if (workbench.panels.isEmpty()) {
            return;
        }

        auto* menu = new QMenu(m_tabBar);
        menu->setAttribute(Qt::WA_StyledBackground, true);
        installRoundedMask(menu, scaledPx(10));
        menu->setToolTipsVisible(true);
        applyPopupMenuTheme(menu);

        auto* contentWidget = new QWidget(menu);
        contentWidget->setObjectName(QStringLiteral("CommandTabWorkbenchPopupGrid"));
        contentWidget->setAttribute(Qt::WA_StyledBackground, true);
        installRoundedMask(contentWidget, scaledPx(8));
        const QColor sectionSurface = withAlpha(
            blendColors(popupMenuSurfaceColor(), m_theme.panelCardBackground, m_theme.isDark ? 0.34 : 0.22),
            m_theme.isDark ? 208 : 224
        );
        const QColor sectionBorder = withAlpha(
            blendColors(popupMenuBorderColor(), m_theme.shellBorder, 0.28),
            196
        );
        const QColor sectionTitle = ensureReadableTextColor(
            sectionSurface,
            popupMenuTextColor(),
            4.5
        );
        contentWidget->setStyleSheet(
            QStringLiteral(
                "QWidget#CommandTabWorkbenchPopupSection {"
                "background: %1;"
                "border: 1px solid %2;"
                "border-radius: 8px;"
                "padding: 4px;"
                "}"
                "QLabel#CommandTabWorkbenchPopupSectionTitle {"
                "color: %3;"
                "background: %1;"
                "border: 1px solid %2;"
                "border-radius: 7px;"
                "padding: 2px 6px;"
                "font-weight: 700;"
                "}"
                "QWidget#CommandTabWorkbenchPopupHeader {"
                "background: transparent;"
                "border: none;"
                "}"
                "QLabel#CommandTabWorkbenchPopupHeaderTitle {"
                "color: %3;"
                "font-weight: 700;"
                "}"
                "QFrame#CommandTabWorkbenchPopupSeparator {"
                "background: %2;"
                "max-height: 1px;"
                "min-height: 1px;"
                "border: none;"
                "}"
            )
                .arg(sectionSurface.name(QColor::HexArgb))
                .arg(sectionBorder.name(QColor::HexArgb))
                .arg(sectionTitle.name(QColor::HexArgb))
        );
        auto* rootLayout = new QVBoxLayout(contentWidget);
        rootLayout->setContentsMargins(
            scaledPx(8),
            scaledPx(8),
            scaledPx(8),
            scaledPx(8)
        );
        rootLayout->setSpacing(scaledPx(6));

        auto* headerWidget = new QWidget(contentWidget);
        headerWidget->setObjectName(QStringLiteral("CommandTabWorkbenchPopupHeader"));
        headerWidget->setAttribute(Qt::WA_StyledBackground, true);
        auto* headerLayout = new QHBoxLayout(headerWidget);
        headerLayout->setContentsMargins(0, 0, 0, 0);
        headerLayout->setSpacing(scaledPx(6));
        auto* headerIcon = new QLabel(headerWidget);
        headerIcon->setFrameStyle(QFrame::NoFrame);
        headerIcon->setFixedSize(scaledPx(20), scaledPx(20));
        const QIcon wbIcon = resolveWorkbenchIcon(workbench);
        if (!wbIcon.isNull()) {
            headerIcon->setPixmap(
                wbIcon.pixmap(QSize(scaledPx(18), scaledPx(18)), QIcon::Normal, QIcon::Off)
            );
        }
        auto* headerTitle = new QLabel(workbench.title, headerWidget);
        headerTitle->setObjectName(QStringLiteral("CommandTabWorkbenchPopupHeaderTitle"));
        headerTitle->setFrameStyle(QFrame::NoFrame);
        QFont headerFont = headerTitle->font();
        headerFont.setBold(true);
        headerTitle->setFont(headerFont);
        headerLayout->addWidget(headerIcon, 0, Qt::AlignVCenter);
        headerLayout->addWidget(headerTitle, 1, Qt::AlignVCenter);
        rootLayout->addWidget(headerWidget);

        const int configuredColumns = std::clamp(m_settingsState.panelDropdownPopupColumns, 1, 8);
        int sectionCount = 0;
        for (const auto& panel : workbench.panels) {
            const QVector<CommandTabCommandEntry> commands = collectPanelActionableCommands(panel, true);
            if (commands.isEmpty()) {
                continue;
            }
            const QString panelKey = panelUsageKey(workbench.id, panel.id);
            const QString panelTitle = normalizedPanelDisplayTitle(
                panel.title.trimmed().isEmpty() ? panel.id : panel.title,
                workbench.id,
                workbench.title
            );
            const QString sectionTitleText = QStringLiteral("%1  (%2)").arg(panelTitle).arg(commands.size());

            auto* sectionWidget = new QWidget(contentWidget);
            sectionWidget->setObjectName(QStringLiteral("CommandTabWorkbenchPopupSection"));
            sectionWidget->setAttribute(Qt::WA_StyledBackground, true);
            installRoundedMask(sectionWidget, scaledPx(8));
            auto* sectionLayout = new QVBoxLayout(sectionWidget);
            sectionLayout->setContentsMargins(0, 0, 0, 0);
            sectionLayout->setSpacing(scaledPx(4));

            auto* titleLabel = new QLabel(sectionTitleText, sectionWidget);
            titleLabel->setObjectName(QStringLiteral("CommandTabWorkbenchPopupSectionTitle"));
            titleLabel->setAttribute(Qt::WA_StyledBackground, true);
            titleLabel->setFrameStyle(QFrame::NoFrame);
            installRoundedMask(titleLabel, scaledPx(7));
            QFont titleFont = titleLabel->font();
            titleFont.setBold(true);
            titleLabel->setFont(titleFont);
            titleLabel->setToolTip(panelTitle);
            sectionLayout->addWidget(titleLabel);

            auto* gridWidget = new QWidget(sectionWidget);
            auto* gridLayout = new QGridLayout(gridWidget);
            gridLayout->setContentsMargins(0, 0, 0, 0);
            gridLayout->setHorizontalSpacing(scaledPx(4));
            gridLayout->setVerticalSpacing(scaledPx(4));
            const int columns = std::max(1, std::min(configuredColumns, static_cast<int>(commands.size())));
            int visibleIndex = 0;
            for (const auto& command : commands) {
                auto* button = createPanelPopupToolButton(command, panelKey, menu, gridWidget);
                if (button == nullptr) {
                    continue;
                }
                const int row = visibleIndex / columns;
                const int column = visibleIndex % columns;
                gridLayout->addWidget(button, row, column);
                ++visibleIndex;
            }
            if (visibleIndex <= 0) {
                sectionWidget->deleteLater();
                continue;
            }

            sectionLayout->addWidget(gridWidget);
            rootLayout->addWidget(sectionWidget);
            ++sectionCount;

            auto* separator = new QFrame(contentWidget);
            separator->setFrameShape(QFrame::HLine);
            separator->setFrameShadow(QFrame::Plain);
            separator->setObjectName(QStringLiteral("CommandTabWorkbenchPopupSeparator"));
            rootLayout->addWidget(separator);
        }

        if (sectionCount > 0) {
            // Remove trailing separator.
            if (QLayoutItem* trailing = rootLayout->takeAt(rootLayout->count() - 1)) {
                if (QWidget* separator = trailing->widget()) {
                    separator->deleteLater();
                }
                delete trailing;
            }
        }

        if (sectionCount <= 0) {
            contentWidget->deleteLater();
            menu->deleteLater();
            return;
        }

        auto* scrollArea = new QScrollArea(menu);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setWidgetResizable(true);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scrollArea->setAttribute(Qt::WA_StyledBackground, true);
        if (scrollArea->viewport() != nullptr) {
            scrollArea->viewport()->setAttribute(Qt::WA_StyledBackground, true);
            installRoundedMask(scrollArea->viewport(), scaledPx(7));
        }
        scrollArea->setWidget(contentWidget);

        const int popupMaxHeight = scaledPx(520);
        const int popupMinWidth = scaledPx(240);
        const int popupMaxWidth = scaledPx(980);
        const QSize contentSize = contentWidget->sizeHint();
        const int popupWidth = std::clamp(contentSize.width() + scaledPx(14), popupMinWidth, popupMaxWidth);
        const int popupHeight = std::min(contentSize.height() + scaledPx(10), popupMaxHeight);
        scrollArea->setFixedSize(popupWidth, popupHeight);

        auto* widgetAction = new QWidgetAction(menu);
        widgetAction->setDefaultWidget(scrollArea);
        menu->addAction(widgetAction);

        const QRect tabRect = m_tabBar->tabRect(index);
        const QPoint popupAnchor = m_tabBar->mapToGlobal(
            QPoint(tabRect.left(), tabRect.bottom() + scaledPx(4))
        );
        menu->exec(popupAnchor);
        menu->deleteLater();
    }

    QToolButton* createPanelExpandButton(
        const QString& panelKey,
        const CommandTabPanelEntry& panel,
        QWidget* parent,
        bool inlineSplit = false
    )
    {
        auto* button = new QToolButton(parent);
        button->setToolTip(QCoreApplication::translate("CommandTabShellWidget", "Show tools in this panel"));
        button->setAutoRaise(false);
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        button->setProperty("commandtabRole", QStringLiteral("panelExpand"));
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setIconSize(QSize(scaledPx(10), scaledPx(10)));
        button->setIcon(buildThemedArrowIcon(m_theme.buttonText, button->iconSize()));
        if (inlineSplit) {
            button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            button->setMinimumSize(QSize(0, 0));
            button->setIconSize(QSize(scaledPx(12), scaledPx(12)));
            button->setIcon(buildThemedArrowIcon(m_theme.buttonText, button->iconSize()));
        } else if (m_settingsState.compactPanelLayout) {
            button->setFixedHeight(std::max(scaledPx(14), button->fontMetrics().lineSpacing()));
            button->setMinimumWidth(scaledPx(14));
        } else {
            button->setFixedHeight(
                std::max(scaledPx(15), button->fontMetrics().lineSpacing() + scaledPx(1))
            );
            button->setMinimumWidth(scaledPx(17));
        }
        connect(button, &QToolButton::clicked, this, [this, panelKey, panel, button]() {
            QMenu* menu = createPanelExpandMenu(panelKey, panel, button);
            if (menu == nullptr) {
                return;
            }
            menu->exec(button->mapToGlobal(QPoint(0, button->height())));
            menu->deleteLater();
        });
        return button;
    }

    QWidget* createPanelPrimarySplitWidget(
        const CommandTabCommandEntry& primaryCommand,
        const QString& panelKey,
        const CommandTabPanelEntry& panel,
        QWidget* parent
    )
    {
        CommandTabCommandEntry compactPrimaryCommand = primaryCommand;
        compactPrimaryCommand.textVisible = false;
        auto* primaryButton = createCommandButton(compactPrimaryCommand, false, panelKey);
        if (primaryButton == nullptr) {
            return nullptr;
        }

        auto* expandButton = createPanelExpandButton(panelKey, panel, parent, true);
        if (expandButton == nullptr) {
            return primaryButton;
        }

        auto* splitWidget = new CommandTabPanelSplitButtonWidget(
            primaryButton,
            expandButton,
            m_settingsState.commandtabScalePercent,
            parent
        );
        splitWidget->setToolTip(
            primaryCommand.text.simplified().isEmpty()
                ? primaryCommand.id
                : primaryCommand.text.simplified()
        );
        // Force full-height column placement so the split uses the full panel body
        // height (top action ~2/3, bottom arrow ~1/3) instead of a single small row.
        splitWidget->setProperty("commandtabColumnType", QStringLiteral("large"));
        return splitWidget;
    }

    QToolButton* createPanelRecentSideButton(
        const CommandTabCommandEntry& command,
        const QString& panelKey,
        QWidget* parent
    )
    {
        if (parent == nullptr || command.id.isEmpty()) {
            return nullptr;
        }

        auto* button = new QToolButton(parent);
        button->setAutoRaise(false);
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setProperty("commandtabRole", QStringLiteral("panelSideTool"));
        button->setProperty("commandtabCommandId", command.id);
        button->setProperty("commandtabHasMenuCommands", !command.menuCommands.isEmpty());
        button->setProperty("commandtabMenuCommandCount", command.menuCommands.size());
        button->setProperty("commandtabCompactIconOnly", true);
        button->setProperty(
            "commandtabPreferredToolButtonStyle",
            static_cast<int>(Qt::ToolButtonIconOnly)
        );
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        const int iconSize = std::clamp(
            scaledPx(std::clamp(m_settingsState.iconOnlySizeSmall, 12, 64)),
            scaledPx(10),
            scaledPx(72)
        );
        const int edge = std::max(scaledPx(28), iconSize + scaledPx(8));
        button->setIconSize(QSize(iconSize, iconSize));
        button->setFixedSize(edge, edge);

        QIcon icon = loadCommandEntryIcon(command);
        if (const QAction* sourceAction = resolveActionForCommandId(command.id)) {
            if (!sourceAction->icon().isNull()) {
                icon = sourceAction->icon();
            }
        }
        if (!icon.isNull()) {
            button->setIcon(icon);
        } else {
            button->setText(commandtabCommandDisplayText(command).left(1));
            button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        }
        button->setToolTip(commandtabCommandDisplayText(command));
        bindWidgetEnabledToCommand(button, command.id);
        connect(button, &QToolButton::clicked, this, [this, panelKey, command]() {
            dispatchCommand(command.id, panelKey);
        });
        return button;
    }

    QWidget* createPanelWidget(const QString& workbenchId, const CommandTabPanelEntry& panel)
    {
        const QString panelKey = panelUsageKey(workbenchId, panel.id);
        auto* panelWidget = new QWidget(this);
        panelWidget->setObjectName(QStringLiteral("CommandTabPanelCard"));
        panelWidget->setAttribute(Qt::WA_StyledBackground, true);
        installRoundedMask(panelWidget, scaledPx(12));
        panelWidget->setProperty("commandtabPanelUsageKey", panelKey);
        panelWidget->setProperty("commandtabWorkbenchId", workbenchId);
        panelWidget->setProperty("commandtabPanelId", panel.id);
        panelWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        auto* panelLayout = new QVBoxLayout(panelWidget);
        const int panelInset = scaledPx(2);
        const int panelContentSpacing = scaledPx(2);
        panelLayout->setContentsMargins(panelInset, panelInset, panelInset, panelInset);
        panelLayout->setSpacing(panelContentSpacing);

        auto* bodyFrame = new CommandTabPanelBodyWidget(&m_theme, m_settingsState, panelWidget);
        installRoundedMask(bodyFrame, scaledPx(8));
        const bool panelDropdownModeEnabled = m_settingsState.panelDropdownModeEnabled;
        if (panelDropdownModeEnabled) {
            const CommandTabCommandEntry primaryCommand = resolvePrimaryPanelCommand(workbenchId, panel);
            if (!primaryCommand.id.isEmpty()) {
                if (auto* splitWidget = createPanelPrimarySplitWidget(primaryCommand, panelKey, panel, bodyFrame)) {
                    bodyFrame->addCommandWidget(splitWidget);
                }
                const QVector<CommandTabCommandEntry> sideRecentCommands = resolveRecentSidePanelCommands(
                    workbenchId,
                    panel,
                    primaryCommand.id
                );
                for (const auto& sideCommand : sideRecentCommands) {
                    if (auto* sideButton = createPanelRecentSideButton(sideCommand, panelKey, bodyFrame)) {
                        bodyFrame->addCommandWidget(sideButton);
                    }
                }
            } else {
                for (const auto& command : panel.commands) {
                    if (command.type == QStringLiteral("separator")) {
                        bodyFrame->addSeparatorWidget(
                            new CommandTabSeparatorWidget(
                                &m_theme,
                                m_settingsState.commandtabScalePercent,
                                bodyFrame
                            )
                        );
                        continue;
                    }

                    auto* button = createCommandButton(command, false, panelKey);
                    if (button == nullptr) {
                        continue;
                    }
                    bodyFrame->addCommandWidget(button);
                }
            }
        } else {
            for (const auto& command : panel.commands) {
                if (command.type == QStringLiteral("separator")) {
                    bodyFrame->addSeparatorWidget(
                        new CommandTabSeparatorWidget(
                            &m_theme,
                            m_settingsState.commandtabScalePercent,
                            bodyFrame
                        )
                    );
                    continue;
                }

                auto* button = createCommandButton(command, false, panelKey);
                if (button == nullptr) {
                    continue;
                }
                bodyFrame->addCommandWidget(button);
            }
        }
        bodyFrame->finalizeLayout();

        const PanelMetrics panelMetrics = collectPanelMetrics(bodyFrame);
        int panelWidth = compactPanelWidthForBody(bodyFrame->sizeHint().width(), panelMetrics);
        int panelInnerWidth = std::max(
            bodyFrame->sizeHint().width(),
            std::max(scaledPx(42), panelWidth - (panelInset * 2))
        );
        auto* footerWidget = new QWidget(panelWidget);
        footerWidget->setObjectName(QStringLiteral("CommandTabPanelFooter"));
        footerWidget->setAttribute(Qt::WA_StyledBackground, true);
        installRoundedMask(footerWidget, scaledPx(12));
        auto* footerLayout = new QHBoxLayout(footerWidget);
        if (m_settingsState.compactPanelLayout) {
            footerLayout->setContentsMargins(scaledPx(6), scaledPx(3), scaledPx(6), scaledPx(3));
            footerLayout->setSpacing(scaledPx(2));
        } else {
            footerLayout->setContentsMargins(scaledPx(9), scaledPx(3), scaledPx(9), scaledPx(3));
            footerLayout->setSpacing(scaledPx(3));
        }

        const int titleHorizontalPadding = m_settingsState.compactPanelLayout
            ? scaledPx(8)
            : scaledPx(10);
        // The body frame keeps its natural content width — the border drawn around
        // it then matches exactly the button area. The panel card and footer may be
        // wider (to give the title label enough readable space); the body is centered
        // inside the card via AlignHCenter rather than being stretched to fill it.
        footerWidget->setFixedWidth(panelInnerWidth);
        bodyFrame->setFixedWidth(panelInnerWidth);

        const QString panelDisplayTitle = normalizedPanelDisplayTitle(
            panel.title,
            workbenchId,
            workbenchTitleById(workbenchId)
        );
        auto* titleLabel = new QLabel(panelDisplayTitle, footerWidget);
        titleLabel->setObjectName(QStringLiteral("CommandTabPanelTitle"));
        titleLabel->setAttribute(Qt::WA_StyledBackground, false);
        titleLabel->setFrameStyle(QFrame::NoFrame);
        {
            QFont titleFont = titleLabel->font();
            titleFont.setPointSize(std::clamp(scaledPx(10), 7, 20));
            titleFont.setWeight(QFont::Bold);
            titleLabel->setFont(titleFont);
        }
        titleLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        titleLabel->setWordWrap(false);
        titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        const int desiredTitleWidth = titleLabel->fontMetrics().horizontalAdvance(panelDisplayTitle) + scaledPx(18);
        panelInnerWidth = std::max(panelInnerWidth, desiredTitleWidth + scaledPx(10));
        footerWidget->setFixedWidth(panelInnerWidth);
        bodyFrame->setFixedWidth(panelInnerWidth);
        const int titleWidth = std::max(
            m_settingsState.compactPanelLayout ? scaledPx(68) : scaledPx(82),
            panelInnerWidth - titleHorizontalPadding
        );
        const int footerHeight = commandtabPanelFooterHeight(titleLabel->fontMetrics());
        const int titleHeight = std::max(scaledPx(18), footerHeight - scaledPx(8));
        titleLabel->setFixedWidth(titleWidth);
        titleLabel->setFixedHeight(titleHeight);
        const QString fullPanelTitle = panel.title.trimmed().isEmpty() ? panelDisplayTitle : panel.title.trimmed();
        titleLabel->setToolTip(fullPanelTitle);
        titleLabel->setText(panelDisplayTitle);
        footerWidget->setFixedHeight(footerHeight);

        const QRect safeRect = footerWidget->rect().adjusted(
            scaledPx(2),
            scaledPx(2),
            -scaledPx(2),
            -scaledPx(2)
        );
        const int safeTitleWidth = std::max(
            scaledPx(42),
            std::min(titleLabel->width(), safeRect.width() - scaledPx(2))
        );
        const int safeTitleHeight = std::max(
            scaledPx(16),
            std::min(titleLabel->height(), safeRect.height() - scaledPx(4))
        );
        titleLabel->setFixedWidth(safeTitleWidth);
        titleLabel->setFixedHeight(safeTitleHeight);
        footerLayout->addWidget(titleLabel, 1, Qt::AlignCenter);

        panelLayout->addWidget(bodyFrame, 0, Qt::AlignHCenter);
        panelLayout->addWidget(footerWidget);
        panelWidth = panelInnerWidth + (panelInset * 2);
        panelWidget->setFixedWidth(panelWidth);
        panelWidget->setFixedHeight(bodyFrame->sizeHint().height() + footerHeight + (panelInset * 2) + panelContentSpacing);
        return panelWidget;
    }

    bool isCommandInQuickAccess(const QString& commandId) const
    {
        for (const auto& qa : m_model.quickAccess) {
            if (qa.id == commandId) {
                return true;
            }
        }
        return false;
    }

    QString workbenchTitleById(const QString& workbenchId) const
    {
        const QString trimmedId = workbenchId.trimmed();
        if (trimmedId.isEmpty()) {
            return QString();
        }
        for (const auto& workbench : m_workbenchEntries) {
            if (workbench.id.trimmed().compare(trimmedId, Qt::CaseInsensitive) == 0) {
                return workbench.title.trimmed();
            }
        }
        return QString();
    }

    bool isTechnicalPrefixToken(const QString& token) const
    {
        static const QSet<QString> technicalPrefixes = {
            QStringLiteral("std"),
            QStringLiteral("draft"),
            QStringLiteral("sketcher"),
            QStringLiteral("partdesign"),
            QStringLiteral("part"),
            QStringLiteral("mesh"),
            QStringLiteral("fem"),
            QStringLiteral("techdraw"),
            QStringLiteral("surface"),
            QStringLiteral("arch"),
            QStringLiteral("bim"),
            QStringLiteral("assembly"),
            QStringLiteral("spreadsheet"),
            QStringLiteral("openscad"),
            QStringLiteral("robot"),
            QStringLiteral("reverseengineering"),
            QStringLiteral("inspection"),
            QStringLiteral("material"),
            QStringLiteral("points"),
            QStringLiteral("ship"),
            QStringLiteral("path"),
            QStringLiteral("raytracing"),
        };
        return technicalPrefixes.contains(token.trimmed().toLower());
    }

    QString stripTechnicalPrefixFromLabel(QString label) const
    {
        label = label.replace(QStringLiteral("&"), QString()).simplified();
        if (label.isEmpty()) {
            return QString();
        }
        QString tokenizedLabel = label;
        tokenizedLabel.replace(QLatin1Char(':'), QLatin1Char(' '));
        tokenizedLabel.replace(QLatin1Char('_'), QLatin1Char(' '));
        tokenizedLabel.replace(QLatin1Char('-'), QLatin1Char(' '));
        const QStringList parts = tokenizedLabel.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (parts.size() <= 1) {
            return label;
        }
        if (!isTechnicalPrefixToken(parts.first())) {
            return label;
        }
        const QString stripped = parts.mid(1).join(QStringLiteral(" ")).simplified();
        return stripped.isEmpty() ? label : stripped;
    }

    QString normalizedPanelDisplayTitle(QString panelTitle, const QString& workbenchId, const QString& workbenchTitle) const
    {
        panelTitle = panelTitle.simplified();
        if (panelTitle.isEmpty()) {
            return QString();
        }

        auto stripWorkbenchSuffix = [&panelTitle](const QString& token) {
            const QString trimmedToken = token.trimmed();
            if (trimmedToken.isEmpty()) {
                return;
            }
            const QStringList suffixes = {
                QStringLiteral(" - %1").arg(trimmedToken),
                QStringLiteral(" (%1)").arg(trimmedToken),
            };
            for (const auto& suffix : suffixes) {
                if (panelTitle.endsWith(suffix, Qt::CaseInsensitive)) {
                    panelTitle.chop(suffix.size());
                    panelTitle = panelTitle.simplified();
                }
            }
        };

        stripWorkbenchSuffix(workbenchId);
        stripWorkbenchSuffix(workbenchTitle);

        if (panelTitle.endsWith(QStringLiteral(" toolbar"), Qt::CaseInsensitive)) {
            const QString trimmed = panelTitle.left(panelTitle.size() - QStringLiteral(" toolbar").size()).simplified();
            if (!trimmed.isEmpty()) {
                panelTitle = trimmed;
            }
        }

        if (panelTitle.startsWith(QStringLiteral("toolbar - "), Qt::CaseInsensitive)) {
            const QString trimmed = panelTitle.mid(QStringLiteral("toolbar - ").size()).simplified();
            if (!trimmed.isEmpty()) {
                panelTitle = trimmed;
            }
        }

        return panelTitle;
    }

    QString humanizedCommandIdLabel(QString commandId) const
    {
        commandId = normalizeActionCandidate(commandId);
        if (commandId.endsWith(QStringLiteral("_ddb"))) {
            commandId.chop(4);
        }
        const qsizetype commaIndex = commandId.indexOf(QStringLiteral(", "));
        if (commaIndex > 0) {
            commandId = commandId.left(commaIndex).trimmed();
        }
        QStringList parts = commandId.split(QLatin1Char('_'), Qt::SkipEmptyParts);
        if (parts.size() > 1 && isTechnicalPrefixToken(parts.first())) {
            parts.removeFirst();
        }
        return parts.join(QStringLiteral(" ")).simplified();
    }

    QString commandHoverLabel(const CommandTabCommandEntry& command, const QAction* sourceAction = nullptr) const
    {
        if (sourceAction != nullptr) {
            const QString actionText = cleanedActionText(const_cast<QAction*>(sourceAction))
                .replace(QStringLiteral("&"), QString())
                .simplified();
            if (!actionText.isEmpty() && !looksLikeTechnicalCommandText(actionText, command.id)) {
                return stripTechnicalPrefixFromLabel(actionText);
            }

            const QString actionToolTip = sourceAction->toolTip()
                .replace(QStringLiteral("&"), QString())
                .simplified();
            if (!actionToolTip.isEmpty() && !looksLikeTechnicalCommandText(actionToolTip, command.id)) {
                return stripTechnicalPrefixFromLabel(actionToolTip);
            }
        }

        QString label = commandtabCommandDisplayText(command).replace(QStringLiteral("&"), QString()).simplified();
        if (label.isEmpty()) {
            label = command.id.trimmed();
        }
        if (looksLikeCommandId(label)) {
            const QString humanized = humanizedCommandIdLabel(command.id);
            if (!humanized.isEmpty()) {
                label = humanized;
            }
        }
        return stripTechnicalPrefixFromLabel(label);
    }

    QWidget* createCommandButton(
        const CommandTabCommandEntry& command,
        bool quickAccess,
        const QString& panelKey = QString()
    )
    {
        if (command.type == QStringLiteral("separator")) {
            return nullptr;
        }

        const QString rawText = command.text.simplified();
        QAction* sourceAction = resolveActionForCommandId(command.id);
        const QIcon runtimeActionIcon =
            (sourceAction != nullptr) ? sourceAction->icon() : QIcon();

        if (quickAccess) {
            auto* button = new QToolButton(this);
            const QString hoverLabel = commandHoverLabel(command, sourceAction);
            button->setToolTip(hoverLabel);
            button->setStatusTip(hoverLabel);
            button->setAccessibleName(hoverLabel);
            button->setToolTipDuration(12000);
            button->setAutoRaise(false);
            button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            QIcon quickIcon = loadCommandEntryIcon(command);
            if (!runtimeActionIcon.isNull()) {
                quickIcon = runtimeActionIcon;
            }
            if (
                (
                    command.id == QStringLiteral("__commandtab_toggle_grid__")
                    || command.id == QStringLiteral("Draft_ToggleGrid")
                )
                && runtimeActionIcon.isNull()
            ) {
                const QStringList fallbackCandidates = {
                    QStringLiteral("Sketcher_GridToggle_Deactivated.svg"),
                    QStringLiteral("Draft_ToggleGrid"),
                    QStringLiteral("view-grid"),
                };
                for (const auto& candidate : fallbackCandidates) {
                    const QIcon candidateIcon = loadIconFromSource(candidate);
                    if (!candidateIcon.isNull()) {
                        quickIcon = candidateIcon;
                        break;
                    }
                }
            }
            if (!quickIcon.isNull()) {
                button->setIcon(quickIcon);
            }
            button->setProperty("commandtabRole", QStringLiteral("quick"));
            button->setProperty("commandtabCommandId", command.id);
            button->setProperty("commandtabHasMenuCommands", !command.menuCommands.isEmpty());
            button->setProperty("commandtabMenuCommandCount", command.menuCommands.size());
            button->setToolButtonStyle(Qt::ToolButtonIconOnly);
            button->setProperty(
                "commandtabPreferredToolButtonStyle",
                static_cast<int>(Qt::ToolButtonIconOnly)
            );
            button->setIconSize(quickAccessIconSize());
            button->setFixedSize(scaledHeaderPx(30), scaledHeaderPx(30));
            if (!command.menuCommands.isEmpty()) {
                auto* menu = new QMenu(button);
                applyPopupMenuTheme(menu);
                for (const auto& menuCommand : command.menuCommands) {
                    if (menuCommand.id.isEmpty()) {
                        continue;
                    }
                    QAction* action = menu->addAction(commandtabCommandDisplayText(menuCommand));
                    const QAction* menuSourceAction = resolveActionForCommandId(menuCommand.id);
                    QIcon menuIcon = loadCommandEntryIcon(menuCommand);
                    if (menuSourceAction != nullptr && !menuSourceAction->icon().isNull()) {
                        menuIcon = menuSourceAction->icon();
                    }
                    if (!menuIcon.isNull()) {
                        action->setIcon(menuIcon);
                    }
                    const QString menuHoverLabel = commandHoverLabel(menuCommand, menuSourceAction);
                    action->setToolTip(menuHoverLabel);
                    action->setStatusTip(menuHoverLabel);
                    bindMenuActionEnabledToCommand(action, menuCommand.id);
                    connect(action, &QAction::triggered, this, [this, panelKey, menuCommand]() {
                        dispatchCommand(menuCommand.id, panelKey);
                    });
                }
                button->setPopupMode(QToolButton::MenuButtonPopup);
                button->setMenu(menu);
            }
            connect(button, &QToolButton::clicked, this, [this, panelKey, command]() {
                dispatchCommand(command.id, panelKey);
            });
            bindWidgetEnabledToCommand(button, command.id);
            return button;
        }

        auto* button = new CommandTabCommandButton(command, &m_theme, this);
        button->setProperty("commandtabCommandId", command.id);
        button->setProperty("commandtabHasMenuCommands", !command.menuCommands.isEmpty());
        button->setProperty("commandtabMenuCommandCount", command.menuCommands.size());
        button->applySettings(m_settingsState);
        const bool isLargeCommand =
            command.size.trimmed().compare(QStringLiteral("large"), Qt::CaseInsensitive) == 0;
        if (!runtimeActionIcon.isNull() && !isLargeCommand) {
            button->setIconOverride(runtimeActionIcon);
        }
        button->setActionResolver([this](const QString& commandId) {
            return resolveActionForCommandId(commandId);
        });
        button->setTriggeredHandler([this, panelKey, command]() {
            dispatchCommand(command.id, panelKey);
        });
        button->setMenuCommandHandler([this, panelKey](const QString& commandId) {
            dispatchCommand(commandId, panelKey);
        });
        button->setContextCommandHandler([this](const QString& cmd) {
            if (m_commandHandler) {
                m_commandHandler(cmd);
            }
        });
        button->setInQuickAccess(isCommandInQuickAccess(command.id));
        bindWidgetEnabledToCommand(button, command.id);
        bindWidgetCheckedToCommand(button, command.id);
        return button;
    }

    QToolButton* createPanelOptionButton(const QString& workbenchId, const QString& panelId, QWidget* parent)
    {
        auto* button = new QToolButton(parent);
        button->setText(QStringLiteral("..."));
        button->setToolTip(QCoreApplication::translate("CommandTabShellWidget", "Customize this panel"));
        button->setAutoRaise(false);
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        button->setProperty("commandtabRole", QStringLiteral("panelOption"));
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        if (m_settingsState.compactPanelLayout) {
            button->setFixedHeight(std::max(scaledPx(14), button->fontMetrics().lineSpacing()));
            button->setMinimumWidth(scaledPx(14));
        } else {
            button->setFixedHeight(
                std::max(scaledPx(15), button->fontMetrics().lineSpacing() + scaledPx(1))
            );
            button->setMinimumWidth(scaledPx(17));
        }
        connect(button, &QToolButton::clicked, this, [this, workbenchId, panelId]() {
            openCustomizationDialog(workbenchId, panelId);
        });
        return button;
    }

    QToolButton* createUtilityButton(
        const QString& commandId,
        const QString& text,
        const QString& toolTip,
        std::function<void()> localHandler = {}
    )
    {
        auto* button = new QToolButton(this);
        button->setText(text);
        button->setToolTip(toolTip);
        button->setAutoRaise(false);
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        button->setProperty("commandtabRole", QStringLiteral("utility"));
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        QFont utilityFont = button->font();
        utilityFont.setPointSize(std::clamp(scaledHeaderPx(10), 7, 20));
        utilityFont.setWeight(QFont::DemiBold);
        button->setFont(utilityFont);
        button->setFixedHeight(
            std::max(scaledHeaderPx(24), button->fontMetrics().lineSpacing() + scaledHeaderPx(4))
        );
        button->setMinimumWidth(scaledHeaderPx(58));
        connect(button, &QToolButton::clicked, this, [this, commandId, localHandler = std::move(localHandler)]() {
            if (localHandler) {
                localHandler();
                return;
            }
            if (m_commandHandler) {
                m_commandHandler(commandId);
            }
        });
        return button;
    }

    void resizeEvent(QResizeEvent* event) override
    {
        QWidget::resizeEvent(event);
        applyShellDisplayScaleMetrics();
    }

    bool eventFilter(QObject* watched, QEvent* e) override
    {
        if (
            watched != nullptr
            && e != nullptr
            && (e->type() == QEvent::Resize || e->type() == QEvent::Show || e->type() == QEvent::Polish)
        ) {
            if (auto* watchedWidget = qobject_cast<QWidget*>(watched)) {
                if (watchedWidget->property("commandtabRoundedMaskRadius").isValid()) {
                    applyRoundedMask(watchedWidget);
                }
                if (watchedWidget == m_tabBar) {
                    updateTabNavigationButtons();
                }
            }
        }

        if (watched == m_tabBar && m_settingsState.ribbonHoverTab) {
            if (e->type() == QEvent::MouseMove) {
                const auto* me = static_cast<QMouseEvent*>(e);
                const int hoveredTab = m_tabBar->tabAt(me->pos());
                if (hoveredTab >= 0 && hoveredTab != m_hoverTabPending) {
                    m_hoverTabPending = hoveredTab;
                    if (m_hoverTabTimer == nullptr) {
                        m_hoverTabTimer = new QTimer(this);
                        m_hoverTabTimer->setSingleShot(true);
                        connect(m_hoverTabTimer, &QTimer::timeout, this, [this]() {
                            if (m_settingsState.ribbonHoverTab
                                && m_hoverTabPending >= 0
                                && m_hoverTabPending < m_tabBar->count()
                                && m_hoverTabPending != m_tabBar->currentIndex()) {
                                m_tabBar->setCurrentIndex(m_hoverTabPending);
                            }
                            m_hoverTabPending = -1;
                        });
                    }
                    m_hoverTabTimer->start(200);
                }
            } else if (e->type() == QEvent::Leave) {
                m_hoverTabPending = -1;
                if (m_hoverTabTimer != nullptr) {
                    m_hoverTabTimer->stop();
                }
            }
        }
        return QWidget::eventFilter(watched, e);
    }

    bool event(QEvent* e) override
    {
        if (e->type() == QEvent::Leave) {
            if (m_settingsState.ribbonAutoHide && !m_ribbonCollapsed
                && !rect().contains(mapFromGlobal(QCursor::pos()))) {
                startAutoHideCountdown();
            }
        } else if (e->type() == QEvent::Enter) {
            cancelAutoHideCountdown();
            if (
                m_settingsState.ribbonAutoHide
                && m_ribbonCollapsed
                && !m_settingsState.tabClickPopupMode
            ) {
                setRibbonCollapsed(false, true);
            }
        }
        return QWidget::event(e);
    }

public:
    QString currentWorkbenchId() const
    {
        if (m_tabBar == nullptr) {
            return QString();
        }
        const int index = m_tabBar->currentIndex();
        if (index < 0 || index >= m_workbenchIds.size()) {
            return QString();
        }
        return m_workbenchIds.at(index);
    }

    void openCustomizationDialog(const QString& workbenchId, const QString& panelId)
    {
        if (m_customizationDialog == nullptr) {
            m_customizationDialog = new CommandTabCustomizationDialog(this);
            m_customizationDialog->setAttribute(Qt::WA_DeleteOnClose, false);
            m_customizationDialog->setCommandHandler([this](const QString& command) {
                if (m_commandHandler) {
                    m_commandHandler(command);
                }
            });
        }

        const QString resolvedWorkbenchId = workbenchId.isEmpty() ? currentWorkbenchId() : workbenchId;
        m_customizationDialog->setTheme(m_theme);
        m_customizationDialog->setModel(m_model);
        m_customizationDialog->setSelection(resolvedWorkbenchId, panelId);
        m_customizationDialog->show();
        m_customizationDialog->raise();
        m_customizationDialog->activateWindow();
    }

    void installRoundedMask(QWidget* widget, int radius)
    {
        if (widget == nullptr) {
            return;
        }
        const int clampedRadius = std::max(0, radius);
        widget->setProperty("commandtabRoundedMaskRadius", clampedRadius);
        if (!widget->property("commandtabRoundedMaskInstalled").toBool()) {
            widget->setProperty("commandtabRoundedMaskInstalled", true);
            widget->installEventFilter(this);
        }
        applyRoundedMask(widget);
    }

    void applyRoundedMask(QWidget* widget) const
    {
        if (widget == nullptr) {
            return;
        }
        const QVariant radiusValue = widget->property("commandtabRoundedMaskRadius");
        if (!radiusValue.isValid()) {
            return;
        }
        const int radius = std::max(0, radiusValue.toInt());
        const QRect r = widget->rect();
        if (radius <= 0 || r.width() <= 2 || r.height() <= 2) {
            widget->clearMask();
            return;
        }
        QPainterPath clipPath;
        clipPath.addRoundedRect(
            QRectF(r.adjusted(0, 0, -1, -1)),
            radius,
            radius
        );
        widget->setMask(QRegion(clipPath.toFillPolygon().toPolygon()));
    }

private:
    QWidget* m_topBarWidget = nullptr;
    QHBoxLayout* m_topBarLayout = nullptr;
    QWidget* m_quickAccessWidget = nullptr;
    QHBoxLayout* m_quickAccessLayout = nullptr;
    QLabel* m_brandWidget = nullptr;
    QWidget* m_utilityWidget = nullptr;
    QHBoxLayout* m_utilityLayout = nullptr;
    CommandTabSettingsState m_settingsState;
    QTabBar* m_tabBar = nullptr;
    QStackedWidget* m_stack = nullptr;
    CommandTabModel::CommandTabTheme m_theme;
    QColor m_tabTextColor;
    QColor m_tabSelectedTextColor;
    QColor m_tabDisabledTextColor;
    CommandTabModel::CommandTabTheme m_baseTheme;
    CommandTabModel m_model;
    QVector<QString> m_workbenchIds;
    QVector<CommandTabWorkbenchEntry> m_workbenchEntries;
    QVector<bool> m_workbenchPagesBuilt;
    QVector<bool> m_workbenchPagesDirty;
    QVector<PageBuildState> m_pageBuildStates;
    std::function<void(const QString&)> m_commandHandler;
    std::function<void()> m_contentChangedHandler;
    CommandTabCustomizationDialog* m_customizationDialog = nullptr;
    CommandTabSettingsDialog* m_settingsDialog = nullptr;
    mutable QHash<QString, QPointer<QAction>> m_actionsByCommandId;
    mutable QSet<QString> m_missingActionCommandIds;
    mutable bool m_actionLookupReady = false;
    mutable bool m_actionLookupRetryInProgress = false;
    QHash<QString, QStringList> m_recentPanelCommandHistory;
    QSet<QString> m_pendingPanelPrimaryRefresh;
    bool m_loadingModel = false;
    bool m_backgroundPageWarmupScheduled = false;
    int m_backgroundWarmupCursor = 0;
    QToolButton* m_gridToggleButton = nullptr;
    QToolButton* m_collapseButton = nullptr;
    QPropertyAnimation* m_stackAnimation = nullptr;
    QTimer* m_autoHideTimer = nullptr;
    QTimer* m_hoverTabTimer = nullptr;
    QTimer* m_tabIconRefreshTimer = nullptr;
    QTimer* m_commandIconRefreshTimer = nullptr;
    int m_pendingCommandIconRefreshPasses = 0;
    int m_hoverTabPending = -1;
    bool m_ribbonCollapsed = false;
    bool m_updatingTabScrollButtonLayout = false;
    int m_adaptiveDensityLevel = 0;
    bool m_applyingDisplayScaleMetrics = false;

public:
    int recommendedDockHeight() const
    {
        const auto* outerLayout = qobject_cast<QVBoxLayout*>(layout());
        const int spacing = outerLayout != nullptr ? outerLayout->spacing() : 0;
        const QMargins margins = outerLayout != nullptr ? outerLayout->contentsMargins() : QMargins();

        int totalHeight = margins.top() + margins.bottom();
        bool hasPreviousSection = false;

        if (m_topBarWidget != nullptr && m_topBarWidget->isVisible()) {
            totalHeight += m_topBarWidget->sizeHint().height();
            hasPreviousSection = true;
        }

        // Stack hidden = fully collapsed (animation already done)
        if (m_stack == nullptr || !m_stack->isVisible()) {
            return totalHeight + scaledPx(4);
        }

        // During animation, maximumHeight holds the current animated value — use it directly
        // so the dock follows each frame instead of jumping at the end
        if (m_stackAnimation != nullptr
            && m_stackAnimation->state() == QAbstractAnimation::Running) {
            const int animH = m_stack->maximumHeight();
            if (animH >= 0 && animH < QWIDGETSIZE_MAX) {
                if (hasPreviousSection && animH > 0) {
                    totalHeight += spacing;
                }
                return totalHeight + animH + scaledPx(4);
            }
        }

        int commandtabContentHeight = 0;
        int horizontalScrollBarReserve = 0;
        bool sawScrollArea = false;

        auto accumulateScrollAreaHeight = [&](
                                              const QScrollArea* scrollArea
                                          ) {
            if (scrollArea == nullptr) {
                return;
            }
            sawScrollArea = true;

            int pageHeight = 0;
            if (const auto* content = scrollArea->widget()) {
                pageHeight = content->sizeHint().height();
            }
            if (pageHeight <= 0) {
                pageHeight = scrollArea->sizeHint().height();
            }
            commandtabContentHeight = std::max(commandtabContentHeight, pageHeight);

            if (const auto* scrollBar = scrollArea->horizontalScrollBar()) {
                int reserve = scrollBar->sizeHint().height();
                if (reserve <= 0) {
                    reserve = style()->pixelMetric(
                        QStyle::PM_ScrollBarExtent,
                        nullptr,
                        scrollArea
                    );
                }
                horizontalScrollBarReserve = std::max(horizontalScrollBarReserve, reserve);
            }
        };

        for (const auto& pageState : m_pageBuildStates) {
            accumulateScrollAreaHeight(pageState.scrollArea);
        }

        if (m_stack != nullptr && m_stack->currentWidget() != nullptr) {
            if (const auto* scrollArea = qobject_cast<QScrollArea*>(m_stack->currentWidget())) {
                accumulateScrollAreaHeight(scrollArea);
            } else {
                commandtabContentHeight = std::max(
                    commandtabContentHeight,
                    m_stack->currentWidget()->sizeHint().height()
                );
            }
        }

        if (sawScrollArea && horizontalScrollBarReserve <= 0) {
            horizontalScrollBarReserve = std::max(
                0,
                style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, this)
            );
        }

        if (commandtabContentHeight > 0) {
            if (hasPreviousSection) {
                totalHeight += spacing;
            }
            totalHeight += commandtabContentHeight + horizontalScrollBarReserve;
        }

        return std::max(scaledPx(206), totalHeight + scaledPx(4));
    }
};
