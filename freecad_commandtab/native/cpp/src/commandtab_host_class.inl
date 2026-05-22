class CommandTabHost final : public QObject
{
public:
    explicit CommandTabHost(QMainWindow* mainWindow)
        : QObject(mainWindow)
        , m_mainWindow(mainWindow)
    {
    }

    ~CommandTabHost() override
    {
        if (m_mainWindow != nullptr) {
            m_mainWindow->removeEventFilter(this);
        }
        if (m_dockWidget != nullptr) {
            m_dockWidget->removeEventFilter(this);
        }

        if (m_mainWindow != nullptr && m_dockWidget != nullptr) {
            m_mainWindow->removeDockWidget(m_dockWidget);
        }
        delete m_dockWidget;
        m_dockWidget = nullptr;
        m_shellWidget = nullptr;
    }

    bool attach(const QString& modelPath)
    {
        if (m_mainWindow == nullptr) {
            setLastError(QStringLiteral("Main window pointer is null"));
            return false;
        }

        if (m_dockWidget == nullptr) {
            const QString dockTitle = QCoreApplication::translate("CommandTabShellWidget", "FreeCAD CommandTab Native");
            m_dockWidget = new QDockWidget(dockTitle, m_mainWindow);
            m_dockWidget->setObjectName(QStringLiteral("FreeCADCommandTabNativeDock"));
            m_dockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
            m_dockWidget->setAllowedAreas(Qt::TopDockWidgetArea);
            m_dockWidget->setTitleBarWidget(new QWidget(m_dockWidget));
            m_shellWidget = new CommandTabShellWidget(m_dockWidget);
            m_shellWidget->setContentChangedHandler([this]() { adjustDockHeight(); });
            m_dockWidget->setWidget(m_shellWidget);
            m_mainWindow->addDockWidget(Qt::TopDockWidgetArea, m_dockWidget);
            installWindowEventHooks();
            scheduleDeferredDockAdjustments();
        }

        m_shellWidget->setCommandHandler([this](const QString& command) {
            if (m_commandCallback == nullptr) {
                return;
            }
            const auto bytes = command.toUtf8();
            m_commandCallback(bytes.constData(), m_userData);
        });

        return reload(modelPath);
    }

    bool attachFromJson(const QByteArray& modelJson)
    {
        if (m_mainWindow == nullptr) {
            setLastError(QStringLiteral("Main window pointer is null"));
            return false;
        }

        if (m_dockWidget == nullptr) {
            const QString dockTitle = QCoreApplication::translate("CommandTabShellWidget", "FreeCAD CommandTab Native");
            m_dockWidget = new QDockWidget(dockTitle, m_mainWindow);
            m_dockWidget->setObjectName(QStringLiteral("FreeCADCommandTabNativeDock"));
            m_dockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
            m_dockWidget->setAllowedAreas(Qt::TopDockWidgetArea);
            m_dockWidget->setTitleBarWidget(new QWidget(m_dockWidget));
            m_shellWidget = new CommandTabShellWidget(m_dockWidget);
            m_shellWidget->setContentChangedHandler([this]() { adjustDockHeight(); });
            m_dockWidget->setWidget(m_shellWidget);
            m_mainWindow->addDockWidget(Qt::TopDockWidgetArea, m_dockWidget);
            installWindowEventHooks();
            scheduleDeferredDockAdjustments();
        }

        m_shellWidget->setCommandHandler([this](const QString& command) {
            if (m_commandCallback == nullptr) {
                return;
            }
            const auto bytes = command.toUtf8();
            m_commandCallback(bytes.constData(), m_userData);
        });

        return reloadFromJson(modelJson);
    }

    bool reload(const QString& modelPath)
    {
        if (m_shellWidget == nullptr) {
            setLastError(QStringLiteral("Native commandtab shell is not attached"));
            return false;
        }

        CommandTabModel model;
        if (!loadCommandTabModelFromJson(modelPath, &model)) {
            return false;
        }
        const bool ok = m_shellWidget->setModel(model);
        adjustDockHeight();
        scheduleDeferredDockAdjustments();
        return ok;
    }

    bool reloadFromJson(const QByteArray& modelJson)
    {
        if (m_shellWidget == nullptr) {
            setLastError(QStringLiteral("Native commandtab shell is not attached"));
            return false;
        }

        CommandTabModel model;
        if (!parseCommandTabModelJson(modelJson, &model)) {
            return false;
        }
        const bool ok = m_shellWidget->setModel(model);
        adjustDockHeight();
        scheduleDeferredDockAdjustments();
        return ok;
    }

    bool reloadFromBootstrap(
        const QString& structurePath,
        const QString& metadataCachePath,
        const QString& activeWorkbenchId,
        bool includeAllPanels,
        const QString& themeConfigJson,
        const QString& settingsJson
    )
    {
        if (m_shellWidget == nullptr) {
            setLastError(QStringLiteral("Native commandtab shell is not attached"));
            return false;
        }
        if (structurePath.trimmed().isEmpty() || metadataCachePath.trimmed().isEmpty()) {
            setLastError(QStringLiteral("Missing structurePath or metadataCachePath for bootstrap reload"));
            return false;
        }

        QJsonObject bootstrap;
        bootstrap.insert(QStringLiteral("kind"), QStringLiteral("bootstrap"));
        bootstrap.insert(QStringLiteral("structurePath"), structurePath);
        bootstrap.insert(QStringLiteral("metadataCachePath"), metadataCachePath);
        bootstrap.insert(QStringLiteral("activeWorkbenchId"), activeWorkbenchId);
        bootstrap.insert(QStringLiteral("includeAllPanels"), includeAllPanels);
        if (!themeConfigJson.trimmed().isEmpty()) {
            const QJsonObject themeObject = parseJsonObjectText(themeConfigJson);
            if (!themeObject.isEmpty()) {
                bootstrap.insert(QStringLiteral("themeConfig"), themeObject);
            }
        }
        if (!settingsJson.trimmed().isEmpty()) {
            const QJsonObject settingsObject = parseJsonObjectText(settingsJson);
            if (!settingsObject.isEmpty()) {
                bootstrap.insert(QStringLiteral("settings"), settingsObject);
            }
        }

        CommandTabModel model;
        if (!buildCommandTabModelFromBootstrap(bootstrap, &model)) {
            return false;
        }
        const bool ok = m_shellWidget->setModel(model);
        adjustDockHeight();
        scheduleDeferredDockAdjustments();
        queueUiHidePass();
        return ok;
    }

    bool setActiveWorkbench(const QString& workbenchId)
    {
        if (m_shellWidget == nullptr) {
            setLastError(QStringLiteral("Native commandtab shell is not attached"));
            return false;
        }
        const bool ok = m_shellWidget->setActiveWorkbench(workbenchId);
        adjustDockHeight();
        queueDockHeightAdjustment();
        return ok;
    }

    bool upsertWorkbenchFromJson(const QByteArray& workbenchJson, bool activate)
    {
        if (m_shellWidget == nullptr) {
            setLastError(QStringLiteral("Native commandtab shell is not attached"));
            return false;
        }

        CommandTabWorkbenchEntry workbench;
        if (!parseWorkbenchJson(workbenchJson, &workbench)) {
            return false;
        }

        const bool ok = m_shellWidget->upsertWorkbench(workbench, activate);
        adjustDockHeight();
        queueDockHeightAdjustment();
        return ok;
    }

    bool upsertWorkbenchFromBootstrap(
        const QString& structurePath,
        const QString& metadataCachePath,
        const QString& workbenchId,
        const QString& themeConfigJson,
        const QString& settingsJson,
        bool activate
    )
    {
        if (m_shellWidget == nullptr) {
            setLastError(QStringLiteral("Native commandtab shell is not attached"));
            return false;
        }
        if (
            structurePath.trimmed().isEmpty()
            || metadataCachePath.trimmed().isEmpty()
            || workbenchId.trimmed().isEmpty()
        ) {
            setLastError(QStringLiteral("Incomplete workbench bootstrap payload"));
            return false;
        }

        QJsonObject bootstrap;
        bootstrap.insert(QStringLiteral("kind"), QStringLiteral("workbench_bootstrap"));
        bootstrap.insert(QStringLiteral("structurePath"), structurePath);
        bootstrap.insert(QStringLiteral("metadataCachePath"), metadataCachePath);
        bootstrap.insert(QStringLiteral("workbenchId"), workbenchId);
        if (!themeConfigJson.trimmed().isEmpty()) {
            const QJsonObject themeObject = parseJsonObjectText(themeConfigJson);
            if (!themeObject.isEmpty()) {
                bootstrap.insert(QStringLiteral("themeConfig"), themeObject);
            }
        }
        if (!settingsJson.trimmed().isEmpty()) {
            const QJsonObject settingsObject = parseJsonObjectText(settingsJson);
            if (!settingsObject.isEmpty()) {
                bootstrap.insert(QStringLiteral("settings"), settingsObject);
            }
        }

        CommandTabWorkbenchEntry workbench;
        if (!buildWorkbenchFromBootstrap(bootstrap, &workbench)) {
            return false;
        }
        const bool ok = m_shellWidget->upsertWorkbench(workbench, activate);
        adjustDockHeight();
        queueDockHeightAdjustment();
        queueUiHidePass();
        return ok;
    }

    bool openCustomizationDialog(const QString& workbenchId, const QString& panelId)
    {
        if (m_shellWidget == nullptr) {
            setLastError(QStringLiteral("Native commandtab shell is not attached"));
            return false;
        }

        m_shellWidget->openCustomizationDialog(workbenchId, panelId);
        return true;
    }

    bool openSettingsDialog(const QByteArray& settingsJson)
    {
        if (m_shellWidget == nullptr) {
            setLastError(QStringLiteral("Native commandtab shell is not attached"));
            return false;
        }

        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(settingsJson, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            setLastError(QStringLiteral("Invalid native commandtab settings JSON: %1").arg(parseError.errorString()));
            return false;
        }

        m_shellWidget->openSettingsDialog(parseSettingsState(document.object()));
        return true;
    }

    bool applySettings(const QByteArray& settingsJson)
    {
        if (m_shellWidget == nullptr) {
            setLastError(QStringLiteral("Native commandtab shell is not attached"));
            return false;
        }

        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(settingsJson, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            setLastError(QStringLiteral("Invalid native commandtab settings JSON: %1").arg(parseError.errorString()));
            return false;
        }

        const bool ok = m_shellWidget->applySettings(parseSettingsState(document.object()));
        adjustDockHeight();
        queueDockHeightAdjustment();
        return ok;
    }

    void setCommandCallback(FreeCADCommandTabNativeCommandCallback callback, void* userData)
    {
        m_commandCallback = callback;
        m_userData = userData;
    }

    void setUiHideControllerEnabled(bool enabled)
    {
        m_uiHideControllerEnabled = enabled;
        if (enabled) {
            queueUiHidePass();
            scheduleDeferredUiHidePasses();
        }
    }

private:
    void hideRedundantWorkbenchUi()
    {
        if (!m_uiHideControllerEnabled || m_mainWindow == nullptr) {
            return;
        }

        static const QSet<QString> protectedToolbarNames = {QStringLiteral("SearchBar")};
        for (auto* toolbar : m_mainWindow->findChildren<QToolBar*>()) {
            if (toolbar == nullptr) {
                continue;
            }
            if (m_dockWidget != nullptr && m_dockWidget->isAncestorOf(toolbar)) {
                continue;
            }
            QWidget* parentWidget = toolbar->parentWidget();
            const QString parentName = parentWidget != nullptr ? parentWidget->objectName().trimmed() : QString();
            const QString toolbarName = toolbar->objectName().trimmed();
            if (parentName == QStringLiteral("statusBar") || parentName == QStringLiteral("StatusBarArea")) {
                toolbar->setEnabled(true);
                toolbar->setVisible(true);
                continue;
            }
            const Qt::ToolBarArea area = m_mainWindow->toolBarArea(toolbar);
            const bool protectedArea =
                area == Qt::LeftToolBarArea
                || area == Qt::RightToolBarArea
                || area == Qt::BottomToolBarArea;
            if (protectedArea || protectedToolbarNames.contains(toolbarName)) {
                toolbar->setEnabled(true);
                toolbar->setVisible(true);
                continue;
            }
            toolbar->setVisible(false);
        }

        for (auto* widget : m_mainWindow->findChildren<QWidget*>()) {
            if (widget == nullptr || widget == m_dockWidget || widget == m_shellWidget) {
                continue;
            }
            if (m_dockWidget != nullptr && m_dockWidget->isAncestorOf(widget)) {
                continue;
            }
            const QString objectName = widget->objectName().trimmed();
            const QString className = widget->metaObject() != nullptr
                ? QString::fromLatin1(widget->metaObject()->className()).trimmed()
                : QString();
            if (
                objectName == QStringLiteral("WbTabBar")
                || objectName == QStringLiteral("WbTabBarMore")
                || className.contains(QStringLiteral("WorkbenchComboBox"))
            ) {
                widget->setVisible(false);
            }
        }
    }

    void queueUiHidePass()
    {
        if (!m_uiHideControllerEnabled || m_uiHideQueued) {
            return;
        }
        m_uiHideQueued = true;
        QTimer::singleShot(0, this, [this]() {
            m_uiHideQueued = false;
            hideRedundantWorkbenchUi();
        });
    }

    void scheduleDeferredUiHidePasses()
    {
        if (!m_uiHideControllerEnabled) {
            return;
        }
        const int delaysMs[] = {25, 80, 180, 320, 700};
        for (int delayMs : delaysMs) {
            QTimer::singleShot(delayMs, this, [this]() {
                hideRedundantWorkbenchUi();
            });
        }
    }

    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (
            event != nullptr
            && (
                watched == m_mainWindow
                || watched == m_dockWidget
            )
        ) {
            switch (event->type()) {
                case QEvent::Show:
                case QEvent::Resize:
                case QEvent::LayoutRequest:
                case QEvent::WindowStateChange:
                case QEvent::Polish:
                case QEvent::PolishRequest:
                    queueDockHeightAdjustment();
                    queueUiHidePass();
                    break;
                default:
                    break;
            }
        }
        return QObject::eventFilter(watched, event);
    }

    void installWindowEventHooks()
    {
        if (m_eventHooksInstalled) {
            return;
        }
        if (m_mainWindow != nullptr) {
            m_mainWindow->installEventFilter(this);
        }
        if (m_dockWidget != nullptr) {
            m_dockWidget->installEventFilter(this);
        }
        m_eventHooksInstalled = true;
        queueUiHidePass();
        scheduleDeferredUiHidePasses();
    }

    void queueDockHeightAdjustment()
    {
        if (m_adjustQueued) {
            return;
        }
        m_adjustQueued = true;
        QTimer::singleShot(0, this, [this]() {
            m_adjustQueued = false;
            adjustDockHeight();
        });
    }

    void scheduleDeferredDockAdjustments()
    {
        queueDockHeightAdjustment();
        const int delaysMs[] = {40, 140, 320};
        for (int delayMs : delaysMs) {
            QTimer::singleShot(delayMs, this, [this]() {
                adjustDockHeight();
            });
        }
    }

    void adjustDockHeight()
    {
        if (m_dockWidget == nullptr || m_shellWidget == nullptr) {
            return;
        }
        const int targetHeight = m_shellWidget->recommendedDockHeight();
        m_dockWidget->setMinimumHeight(targetHeight);
        m_dockWidget->setMaximumHeight(targetHeight);
        m_dockWidget->updateGeometry();
    }

    QMainWindow* m_mainWindow = nullptr;
    QDockWidget* m_dockWidget = nullptr;
    CommandTabShellWidget* m_shellWidget = nullptr;
    FreeCADCommandTabNativeCommandCallback m_commandCallback = nullptr;
    void* m_userData = nullptr;
    bool m_eventHooksInstalled = false;
    bool m_adjustQueued = false;
    bool m_uiHideControllerEnabled = true;
    bool m_uiHideQueued = false;
};
