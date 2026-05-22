class CommandTabCustomizationDialog final : public QDialog
{
public:
    explicit CommandTabCustomizationDialog(QWidget* parent = nullptr)
        : QDialog(parent)
    {
        auto trLabel = [](const char* text) {
            return QCoreApplication::translate("CommandTabCustomizationDialog", text);
        };

        setObjectName(QStringLiteral("CommandTabCustomizationDialog"));
        setWindowTitle(trLabel("CommandTab customization"));
        setModal(false);
        resize(1080, 720);
        setMinimumSize(860, 580);

        auto* rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(12, 12, 12, 12);
        rootLayout->setSpacing(10);

        auto* headerLabel = new QLabel(trLabel("Customize the native commandtab layout"), this);
        headerLabel->setObjectName(QStringLiteral("CommandTabCustomizationHeader"));
        rootLayout->addWidget(headerLabel);

        m_tabWidget = new QTabWidget(this);
        rootLayout->addWidget(m_tabWidget, 1);

        auto* layoutTab = new QWidget(m_tabWidget);
        auto* layoutTabLayout = new QVBoxLayout(layoutTab);
        layoutTabLayout->setContentsMargins(0, 0, 0, 0);
        layoutTabLayout->setSpacing(10);

        auto* layoutSplitter = new QSplitter(Qt::Horizontal, layoutTab);
        layoutTabLayout->addWidget(layoutSplitter, 1);

        auto* workbenchPane = new QWidget(layoutSplitter);
        auto* workbenchLayout = new QVBoxLayout(workbenchPane);
        workbenchLayout->setContentsMargins(0, 0, 0, 0);
        workbenchLayout->setSpacing(6);
        workbenchLayout->addWidget(new QLabel(trLabel("Workbenches"), workbenchPane));
        m_workbenchList = new QListWidget(workbenchPane);
        workbenchLayout->addWidget(m_workbenchList, 1);
        layoutSplitter->addWidget(workbenchPane);

        auto* panelPane = new QWidget(layoutSplitter);
        auto* panelLayout = new QVBoxLayout(panelPane);
        panelLayout->setContentsMargins(0, 0, 0, 0);
        panelLayout->setSpacing(6);
        panelLayout->addWidget(new QLabel(trLabel("Panels"), panelPane));
        m_panelList = new QListWidget(panelPane);
        panelLayout->addWidget(m_panelList, 1);

        auto* panelButtonRow = new QHBoxLayout();
        panelButtonRow->setContentsMargins(0, 0, 0, 0);
        panelButtonRow->setSpacing(6);
        auto* panelNewButton = new QPushButton(trLabel("New"), panelPane);
        auto* panelDuplicateButton = new QPushButton(trLabel("Duplicate"), panelPane);
        auto* panelUpButton = new QPushButton(trLabel("Up"), panelPane);
        auto* panelDownButton = new QPushButton(trLabel("Down"), panelPane);
        auto* panelHideButton = new QPushButton(trLabel("Hide"), panelPane);
        panelButtonRow->addWidget(panelNewButton);
        panelButtonRow->addWidget(panelDuplicateButton);
        panelButtonRow->addWidget(panelUpButton);
        panelButtonRow->addWidget(panelDownButton);
        panelButtonRow->addWidget(panelHideButton);
        panelLayout->addLayout(panelButtonRow);

        panelLayout->addWidget(new QLabel(trLabel("Panel title"), panelPane));
        m_panelTitleEdit = new QLineEdit(panelPane);
        panelLayout->addWidget(m_panelTitleEdit);
        panelLayout->addWidget(new QLabel(trLabel("Panel kind"), panelPane));
        m_panelSourceTypeCombo = new QComboBox(panelPane);
        m_panelSourceTypeCombo->addItem(trLabel("Toolbar"), QStringLiteral("toolbar"));
        m_panelSourceTypeCombo->addItem(trLabel("Custom"), QStringLiteral("custom"));
        m_panelSourceTypeCombo->addItem(trLabel("New"), QStringLiteral("new"));
        panelLayout->addWidget(m_panelSourceTypeCombo);
        panelLayout->addWidget(new QLabel(trLabel("Panel scope"), panelPane));
        m_panelSourceWorkbenchCombo = new QComboBox(panelPane);
        panelLayout->addWidget(m_panelSourceWorkbenchCombo);
        layoutSplitter->addWidget(panelPane);

        auto* commandPane = new QWidget(layoutSplitter);
        auto* commandLayout = new QVBoxLayout(commandPane);
        commandLayout->setContentsMargins(0, 0, 0, 0);
        commandLayout->setSpacing(6);
        commandLayout->addWidget(new QLabel(trLabel("Commands"), commandPane));
        m_commandList = new QListWidget(commandPane);
        commandLayout->addWidget(m_commandList, 1);

        auto* commandButtonRow = new QHBoxLayout();
        commandButtonRow->setContentsMargins(0, 0, 0, 0);
        commandButtonRow->setSpacing(6);
        auto* commandUpButton = new QPushButton(trLabel("Up"), commandPane);
        auto* commandDownButton = new QPushButton(trLabel("Down"), commandPane);
        auto* commandRemoveButton = new QPushButton(trLabel("Remove"), commandPane);
        auto* addSeparatorButton = new QPushButton(trLabel("Separator"), commandPane);
        commandButtonRow->addWidget(commandUpButton);
        commandButtonRow->addWidget(commandDownButton);
        commandButtonRow->addWidget(commandRemoveButton);
        commandButtonRow->addWidget(addSeparatorButton);
        commandLayout->addLayout(commandButtonRow);

        commandLayout->addWidget(new QLabel(trLabel("Command text"), commandPane));
        m_commandTextEdit = new QLineEdit(commandPane);
        commandLayout->addWidget(m_commandTextEdit);

        commandLayout->addWidget(new QLabel(trLabel("Command size"), commandPane));
        m_commandSizeCombo = new QComboBox(commandPane);
        m_commandSizeCombo->addItem(trLabel("Small"), QStringLiteral("small"));
        m_commandSizeCombo->addItem(trLabel("Medium"), QStringLiteral("medium"));
        m_commandSizeCombo->addItem(trLabel("Large"), QStringLiteral("large"));
        commandLayout->addWidget(m_commandSizeCombo);

        commandLayout->addWidget(new QLabel(trLabel("Command source workbench"), commandPane));
        m_commandSourceWorkbenchCombo = new QComboBox(commandPane);
        commandLayout->addWidget(m_commandSourceWorkbenchCombo);

        commandLayout->addWidget(new QLabel(trLabel("Command source toolbar"), commandPane));
        m_commandSourceToolbarEdit = new QLineEdit(commandPane);
        commandLayout->addWidget(m_commandSourceToolbarEdit);

        commandLayout->addWidget(new QLabel(trLabel("Add command to panel"), commandPane));
        auto* addCommandRow = new QHBoxLayout();
        addCommandRow->setContentsMargins(0, 0, 0, 0);
        addCommandRow->setSpacing(6);
        m_panelCommandCatalog = new QComboBox(commandPane);
        auto* addCommandButton = new QPushButton(trLabel("Add"), commandPane);
        addCommandRow->addWidget(m_panelCommandCatalog, 1);
        addCommandRow->addWidget(addCommandButton);
        commandLayout->addLayout(addCommandRow);
        layoutSplitter->addWidget(commandPane);

        m_tabWidget->addTab(layoutTab, trLabel("Layout"));

        auto* quickTab = new QWidget(m_tabWidget);
        auto* quickLayout = new QVBoxLayout(quickTab);
        quickLayout->setContentsMargins(0, 0, 0, 0);
        quickLayout->setSpacing(8);
        quickLayout->addWidget(new QLabel(trLabel("Quick access"), quickTab));
        m_quickAccessList = new QListWidget(quickTab);
        quickLayout->addWidget(m_quickAccessList, 1);

        auto* quickButtonRow = new QHBoxLayout();
        quickButtonRow->setContentsMargins(0, 0, 0, 0);
        quickButtonRow->setSpacing(6);
        auto* quickUpButton = new QPushButton(trLabel("Up"), quickTab);
        auto* quickDownButton = new QPushButton(trLabel("Down"), quickTab);
        auto* quickRemoveButton = new QPushButton(trLabel("Remove"), quickTab);
        quickButtonRow->addWidget(quickUpButton);
        quickButtonRow->addWidget(quickDownButton);
        quickButtonRow->addWidget(quickRemoveButton);
        quickLayout->addLayout(quickButtonRow);

        quickLayout->addWidget(new QLabel(trLabel("Add command to quick access"), quickTab));
        auto* quickAddRow = new QHBoxLayout();
        quickAddRow->setContentsMargins(0, 0, 0, 0);
        quickAddRow->setSpacing(6);
        m_quickCommandCatalog = new QComboBox(quickTab);
        auto* quickAddButton = new QPushButton(trLabel("Add"), quickTab);
        quickAddRow->addWidget(m_quickCommandCatalog, 1);
        quickAddRow->addWidget(quickAddButton);
        quickLayout->addLayout(quickAddRow);
        m_tabWidget->addTab(quickTab, trLabel("Quick Access"));

        auto* visibilityTab = new QWidget(m_tabWidget);
        auto* visibilityLayout = new QHBoxLayout(visibilityTab);
        visibilityLayout->setContentsMargins(0, 0, 0, 0);
        visibilityLayout->setSpacing(10);

        auto* workbenchVisibilityPane = new QWidget(visibilityTab);
        auto* workbenchVisibilityLayout = new QVBoxLayout(workbenchVisibilityPane);
        workbenchVisibilityLayout->setContentsMargins(0, 0, 0, 0);
        workbenchVisibilityLayout->setSpacing(6);
        workbenchVisibilityLayout->addWidget(new QLabel(trLabel("Workbench visibility"), workbenchVisibilityPane));
        m_workbenchVisibilityList = new QListWidget(workbenchVisibilityPane);
        m_workbenchVisibilityList->setAlternatingRowColors(true);
        workbenchVisibilityLayout->addWidget(m_workbenchVisibilityList, 1);
        auto* workbenchVisibilityButtonRow = new QHBoxLayout();
        workbenchVisibilityButtonRow->setContentsMargins(0, 0, 0, 0);
        workbenchVisibilityButtonRow->setSpacing(6);
        auto* showAllWorkbenchesButton = new QPushButton(trLabel("Show all"), workbenchVisibilityPane);
        auto* hideAllWorkbenchesButton = new QPushButton(trLabel("Hide all"), workbenchVisibilityPane);
        workbenchVisibilityButtonRow->addWidget(showAllWorkbenchesButton);
        workbenchVisibilityButtonRow->addWidget(hideAllWorkbenchesButton);
        workbenchVisibilityLayout->addLayout(workbenchVisibilityButtonRow);
        visibilityLayout->addWidget(workbenchVisibilityPane, 1);

        auto* panelVisibilityPane = new QWidget(visibilityTab);
        auto* panelVisibilityLayout = new QVBoxLayout(panelVisibilityPane);
        panelVisibilityLayout->setContentsMargins(0, 0, 0, 0);
        panelVisibilityLayout->setSpacing(6);
        panelVisibilityLayout->addWidget(new QLabel(trLabel("Panel visibility"), panelVisibilityPane));
        m_panelVisibilityList = new QListWidget(panelVisibilityPane);
        m_panelVisibilityList->setAlternatingRowColors(true);
        panelVisibilityLayout->addWidget(m_panelVisibilityList, 1);
        auto* panelVisibilityButtonRow = new QHBoxLayout();
        panelVisibilityButtonRow->setContentsMargins(0, 0, 0, 0);
        panelVisibilityButtonRow->setSpacing(6);
        auto* showAllPanelsButton = new QPushButton(trLabel("Show all"), panelVisibilityPane);
        auto* hideAllPanelsButton = new QPushButton(trLabel("Hide all"), panelVisibilityPane);
        panelVisibilityButtonRow->addWidget(showAllPanelsButton);
        panelVisibilityButtonRow->addWidget(hideAllPanelsButton);
        panelVisibilityLayout->addLayout(panelVisibilityButtonRow);
        visibilityLayout->addWidget(panelVisibilityPane, 1);
        m_tabWidget->addTab(visibilityTab, trLabel("Visibility"));

        auto* dropdownTab = new QWidget(m_tabWidget);
        auto* dropdownLayout = new QHBoxLayout(dropdownTab);
        dropdownLayout->setContentsMargins(0, 0, 0, 0);
        dropdownLayout->setSpacing(10);

        auto* dropdownListPane = new QWidget(dropdownTab);
        auto* dropdownListLayout = new QVBoxLayout(dropdownListPane);
        dropdownListLayout->setContentsMargins(0, 0, 0, 0);
        dropdownListLayout->setSpacing(6);
        dropdownListLayout->addWidget(new QLabel(trLabel("Dropdown buttons"), dropdownListPane));
        m_dropdownList = new QListWidget(dropdownListPane);
        dropdownListLayout->addWidget(m_dropdownList, 1);
        auto* dropdownButtonRow = new QHBoxLayout();
        dropdownButtonRow->setContentsMargins(0, 0, 0, 0);
        dropdownButtonRow->setSpacing(6);
        auto* newDropdownButton = new QPushButton(trLabel("New"), dropdownListPane);
        auto* removeDropdownButton = new QPushButton(trLabel("Remove"), dropdownListPane);
        dropdownButtonRow->addWidget(newDropdownButton);
        dropdownButtonRow->addWidget(removeDropdownButton);
        dropdownListLayout->addLayout(dropdownButtonRow);
        dropdownLayout->addWidget(dropdownListPane, 1);

        auto* dropdownEditorPane = new QWidget(dropdownTab);
        auto* dropdownEditorLayout = new QVBoxLayout(dropdownEditorPane);
        dropdownEditorLayout->setContentsMargins(0, 0, 0, 0);
        dropdownEditorLayout->setSpacing(6);
        dropdownEditorLayout->addWidget(new QLabel(trLabel("Dropdown name"), dropdownEditorPane));
        m_dropdownNameEdit = new QLineEdit(dropdownEditorPane);
        dropdownEditorLayout->addWidget(m_dropdownNameEdit);
        dropdownEditorLayout->addWidget(new QLabel(trLabel("Commands"), dropdownEditorPane));
        m_dropdownCommandList = new QListWidget(dropdownEditorPane);
        dropdownEditorLayout->addWidget(m_dropdownCommandList, 1);
        auto* dropdownCommandButtons = new QHBoxLayout();
        dropdownCommandButtons->setContentsMargins(0, 0, 0, 0);
        dropdownCommandButtons->setSpacing(6);
        auto* dropdownCommandUpButton = new QPushButton(trLabel("Up"), dropdownEditorPane);
        auto* dropdownCommandDownButton = new QPushButton(trLabel("Down"), dropdownEditorPane);
        auto* dropdownCommandRemoveButton = new QPushButton(trLabel("Remove"), dropdownEditorPane);
        dropdownCommandButtons->addWidget(dropdownCommandUpButton);
        dropdownCommandButtons->addWidget(dropdownCommandDownButton);
        dropdownCommandButtons->addWidget(dropdownCommandRemoveButton);
        dropdownEditorLayout->addLayout(dropdownCommandButtons);
        auto* dropdownAddRow = new QHBoxLayout();
        dropdownAddRow->setContentsMargins(0, 0, 0, 0);
        dropdownAddRow->setSpacing(6);
        m_dropdownCommandCatalog = new QComboBox(dropdownEditorPane);
        auto* addDropdownCommandButton = new QPushButton(trLabel("Add"), dropdownEditorPane);
        dropdownAddRow->addWidget(m_dropdownCommandCatalog, 1);
        dropdownAddRow->addWidget(addDropdownCommandButton);
        dropdownEditorLayout->addLayout(dropdownAddRow);
        dropdownLayout->addWidget(dropdownEditorPane, 1);
        m_tabWidget->addTab(dropdownTab, trLabel("Dropdowns"));

        auto* storageTab = new QWidget(m_tabWidget);
        auto* storageLayout = new QVBoxLayout(storageTab);
        storageLayout->setContentsMargins(0, 0, 0, 0);
        storageLayout->setSpacing(8);
        storageLayout->addWidget(new QLabel(trLabel("Layout maintenance"), storageTab));

        auto* exportLayoutButton = new QPushButton(trLabel("Export Layout"), storageTab);
        auto* importLayoutButton = new QPushButton(trLabel("Import Layout"), storageTab);
        auto* restoreBackupButton = new QPushButton(trLabel("Restore Backup"), storageTab);
        auto* resetLayoutButton = new QPushButton(trLabel("Reset Default"), storageTab);
        auto* reloadLayoutButton = new QPushButton(trLabel("Reload CommandTab"), storageTab);
        storageLayout->addWidget(exportLayoutButton);
        storageLayout->addWidget(importLayoutButton);
        storageLayout->addWidget(restoreBackupButton);
        storageLayout->addWidget(resetLayoutButton);
        storageLayout->addWidget(reloadLayoutButton);
        storageLayout->addStretch(1);
        m_tabWidget->addTab(storageTab, trLabel("Storage"));

        auto* footerLayout = new QHBoxLayout();
        footerLayout->setContentsMargins(0, 0, 0, 0);
        footerLayout->setSpacing(8);
        m_undoButton = new QPushButton(trLabel("Undo"), this);
        m_undoButton->setObjectName(QStringLiteral("CommandTabUndoButton"));
        m_undoButton->setEnabled(false);
        m_undoButton->setShortcut(QKeySequence::Undo);
        m_redoButton = new QPushButton(trLabel("Redo"), this);
        m_redoButton->setObjectName(QStringLiteral("CommandTabRedoButton"));
        m_redoButton->setEnabled(false);
        m_redoButton->setShortcut(QKeySequence::Redo);
        footerLayout->addWidget(m_undoButton);
        footerLayout->addWidget(m_redoButton);
        footerLayout->addStretch(1);
        auto* applyButton = new QPushButton(trLabel("Apply"), this);
        auto* saveButton = new QPushButton(trLabel("Save and Close"), this);
        auto* closeButton = new QPushButton(trLabel("Close"), this);
        footerLayout->addWidget(applyButton);
        footerLayout->addWidget(saveButton);
        footerLayout->addWidget(closeButton);
        rootLayout->addLayout(footerLayout);

        connect(m_undoButton, &QPushButton::clicked, this, [this]() { undo(); });
        connect(m_redoButton, &QPushButton::clicked, this, [this]() { redo(); });

        connect(m_workbenchList, &QListWidget::currentRowChanged, this, [this](int) {
            if (m_updatingUi) {
                return;
            }
            populatePanelList(QString());
        });
        connect(m_panelList, &QListWidget::currentRowChanged, this, [this](int) {
            if (m_updatingUi) {
                return;
            }
            populateCommandList(QString());
            refreshPanelEditor();
        });
        connect(m_commandList, &QListWidget::currentRowChanged, this, [this](int) {
            if (m_updatingUi) {
                return;
            }
            refreshCommandEditor();
        });
        connect(m_quickAccessList, &QListWidget::currentRowChanged, this, [this](int) {
            if (m_updatingUi) {
                return;
            }
            refreshQuickAccessButtons();
        });
        connect(m_workbenchVisibilityList, &QListWidget::currentRowChanged, this, [this](int) {
            if (!m_updatingUi) {
                refreshWorkbenchVisibilityButtons();
            }
        });
        connect(m_workbenchVisibilityList, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) {
            applyWorkbenchVisibilityItemState(item);
        });
        connect(m_panelVisibilityList, &QListWidget::currentRowChanged, this, [this](int) {
            if (!m_updatingUi) {
                refreshPanelVisibilityButtons();
            }
        });
        connect(m_panelVisibilityList, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) {
            applyPanelVisibilityItemState(item);
        });
        connect(m_dropdownList, &QListWidget::currentRowChanged, this, [this](int) {
            if (!m_updatingUi) {
                populateDropdownCommandList(QString());
                refreshDropdownEditor();
            }
        });
        connect(m_dropdownCommandList, &QListWidget::currentRowChanged, this, [this](int) {
            if (!m_updatingUi) {
                refreshDropdownEditor();
            }
        });
        connect(panelNewButton, &QPushButton::clicked, this, [this]() { createPanel(); });
        connect(panelDuplicateButton, &QPushButton::clicked, this, [this]() { duplicatePanel(); });
        connect(panelUpButton, &QPushButton::clicked, this, [this]() { moveSelectedPanel(-1); });
        connect(panelDownButton, &QPushButton::clicked, this, [this]() { moveSelectedPanel(1); });
        connect(panelHideButton, &QPushButton::clicked, this, [this]() { hideSelectedPanel(); });
        connect(commandUpButton, &QPushButton::clicked, this, [this]() { moveSelectedCommand(-1); });
        connect(commandDownButton, &QPushButton::clicked, this, [this]() { moveSelectedCommand(1); });
        connect(commandRemoveButton, &QPushButton::clicked, this, [this]() { removeSelectedCommand(); });
        connect(addSeparatorButton, &QPushButton::clicked, this, [this]() { addSeparatorToPanel(); });
        connect(addCommandButton, &QPushButton::clicked, this, [this]() { addCatalogCommandToPanel(); });
        connect(quickUpButton, &QPushButton::clicked, this, [this]() { moveQuickAccessCommand(-1); });
        connect(quickDownButton, &QPushButton::clicked, this, [this]() { moveQuickAccessCommand(1); });
        connect(quickRemoveButton, &QPushButton::clicked, this, [this]() { removeQuickAccessCommand(); });
        connect(quickAddButton, &QPushButton::clicked, this, [this]() { addCatalogCommandToQuickAccess(); });
        connect(showAllWorkbenchesButton, &QPushButton::clicked, this, [this]() { setAllWorkbenchVisibility(true); });
        connect(hideAllWorkbenchesButton, &QPushButton::clicked, this, [this]() { setAllWorkbenchVisibility(false); });
        connect(showAllPanelsButton, &QPushButton::clicked, this, [this]() { setAllPanelVisibility(true); });
        connect(hideAllPanelsButton, &QPushButton::clicked, this, [this]() { setAllPanelVisibility(false); });
        connect(newDropdownButton, &QPushButton::clicked, this, [this]() { createDropdown(); });
        connect(removeDropdownButton, &QPushButton::clicked, this, [this]() { removeDropdown(); });
        connect(m_dropdownNameEdit, &QLineEdit::editingFinished, this, [this]() { applyDropdownNameEdit(); });
        connect(addDropdownCommandButton, &QPushButton::clicked, this, [this]() { addCatalogCommandToDropdown(); });
        connect(dropdownCommandUpButton, &QPushButton::clicked, this, [this]() { moveSelectedDropdownCommand(-1); });
        connect(dropdownCommandDownButton, &QPushButton::clicked, this, [this]() { moveSelectedDropdownCommand(1); });
        connect(dropdownCommandRemoveButton, &QPushButton::clicked, this, [this]() { removeSelectedDropdownCommand(); });
        connect(exportLayoutButton, &QPushButton::clicked, this, [this]() { exportLayout(); });
        connect(importLayoutButton, &QPushButton::clicked, this, [this]() { importLayout(); });
        connect(restoreBackupButton, &QPushButton::clicked, this, [this]() { triggerSimpleCommand(QStringLiteral("__commandtab_design_restore__")); });
        connect(resetLayoutButton, &QPushButton::clicked, this, [this]() { triggerSimpleCommand(QStringLiteral("__commandtab_design_reset__")); });
        connect(reloadLayoutButton, &QPushButton::clicked, this, [this]() { triggerSimpleCommand(QStringLiteral("__commandtab_design_reload__")); });
        connect(m_panelTitleEdit, &QLineEdit::editingFinished, this, [this]() { applyPanelTitleEdit(); });
        connect(
            m_panelSourceTypeCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this](int) { applyPanelSourceTypeEdit(); }
        );
        connect(
            m_panelSourceWorkbenchCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this](int) { applyPanelSourceWorkbenchEdit(); }
        );
        connect(m_commandTextEdit, &QLineEdit::editingFinished, this, [this]() { applyCommandTextEdit(); });
        connect(
            m_commandSizeCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this](int) { applyCommandSizeEdit(); }
        );
        connect(
            m_commandSourceWorkbenchCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this](int) { applyCommandSourceWorkbenchEdit(); }
        );
        connect(
            m_commandSourceToolbarEdit,
            &QLineEdit::editingFinished,
            this,
            [this]() { applyCommandSourceToolbarEdit(); }
        );
        connect(applyButton, &QPushButton::clicked, this, [this]() { submitCustomization(QStringLiteral("__commandtab_design_apply__")); });
        connect(saveButton, &QPushButton::clicked, this, [this]() {
            submitCustomization(QStringLiteral("__commandtab_design_save__"));
            accept();
        });
        connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    }

    void setTheme(const CommandTabModel::CommandTabTheme& theme)
    {
        m_theme = theme;
        const QColor textColor = m_theme.buttonText;
        const QColor fieldText = textColor;
        const QColor mutedText = textColor;
        const QColor selectedSurface = withAlpha(
            blendColors(m_theme.quickHoverBackground, m_theme.panelCardBackground, 0.24),
            m_theme.isDark ? 240 : 248
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

        auto applyIndexedQss = [](QString style, const std::initializer_list<QString>& values) {
            int index = static_cast<int>(values.size());
            for (auto it = values.end(); it != values.begin();) {
                --it;
                style.replace(QStringLiteral("%") + QString::number(index), *it);
                --index;
            }
            return style;
        };

        setStyleSheet(
            applyIndexedQss(QStringLiteral(R"(
QDialog#CommandTabCustomizationDialog {
    background: %1;
    color: %2;
}
QDialog#CommandTabCustomizationDialog,
QDialog#CommandTabCustomizationDialog * {
    color: %2;
}
QLabel#CommandTabCustomizationHeader {
    font-size: 15px;
    font-weight: 700;
    padding-bottom: 2px;
}
QTabWidget::pane {
    border: 1px solid %3;
    background: %4;
    border-radius: 8px;
}
QTabBar::tab {
    color: %2;
    background: %5;
    padding: 8px 12px;
    margin-right: 4px;
    border: 1px solid %6;
    border-bottom: none;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
}
QTabBar::tab:selected {
    color: %2;
    background: %4;
    border-color: %3;
}
QListWidget, QLineEdit, QComboBox {
    background: %5;
    color: %7;
    border: 1px solid %6;
    border-radius: 6px;
    padding: 4px;
}
QListWidget::viewport {
    background: %5;
    border-radius: 5px;
}
QListWidget::item:selected {
    background: %8;
    color: %9;
    border: 1px solid %10;
    border-radius: 4px;
}
QComboBox QAbstractItemView {
    background: %5;
    color: %7;
    border: 1px solid %6;
    selection-background-color: %8;
    selection-color: %9;
}
QPushButton {
    background: %5;
    color: %7;
    border: 1px solid %6;
    border-radius: 6px;
    padding: 6px 10px;
}
QPushButton:hover {
    background: %11;
    border-color: %10;
}
QLabel:disabled, QListWidget:disabled, QLineEdit:disabled,
QComboBox:disabled, QPushButton:disabled {
    color: %12;
}
QSplitter::handle {
    background: %6;
    width: 1px;
}
            )"),
                            {
                                m_theme.shellBackground.name(),
                                textColor.name(QColor::HexArgb),
                                m_theme.panelCardBorder.name(),
                                m_theme.panelCardBackground.name(),
                                m_theme.quickBackground.name(),
                                m_theme.quickBorder.name(),
                                fieldText.name(QColor::HexArgb),
                                selectedSurface.name(QColor::HexArgb),
                                selectedText.name(QColor::HexArgb),
                                m_theme.quickHoverBorder.name(),
                                m_theme.quickHoverBackground.name(),
                                mutedText.name(QColor::HexArgb),
                            })
        );
    }

    void setModel(const CommandTabModel& model)
    {
        const QString selectedWorkbench = currentWorkbenchId();
        const QString selectedPanel = currentPanelId();
        const QString selectedCommand = currentCommandId();
        const QString selectedQuickCommand = currentQuickAccessCommandId();

        m_model = model;
        rebuildCommandCatalog();

        m_updatingUi = true;
        populateWorkbenchList(selectedWorkbench);
        populateQuickAccessList(selectedQuickCommand);
        populateWorkbenchVisibilityList();
        populatePanelVisibilityList();
        populateDropdownList(QString());
        m_updatingUi = false;

        populatePanelList(selectedPanel);
        populateCommandList(selectedCommand);
        populateDropdownCommandList(QString());
        refreshPanelEditor();
        refreshCommandEditor();
        refreshQuickAccessButtons();
        refreshWorkbenchVisibilityButtons();
        refreshPanelVisibilityButtons();
        refreshDropdownEditor();
    }

    void setCommandHandler(std::function<void(const QString&)> handler)
    {
        m_commandHandler = std::move(handler);
    }

    void setSelection(const QString& workbenchId, const QString& panelId)
    {
        selectListItemById(m_workbenchList, workbenchId);
        populatePanelList(panelId);
        populateCommandList(QString());
    }

private:
    static constexpr int k_maxUndoDepth = 50;

    void refreshUndoRedoButtons()
    {
        if (m_undoButton != nullptr) {
            m_undoButton->setEnabled(!m_undoStack.isEmpty());
        }
        if (m_redoButton != nullptr) {
            m_redoButton->setEnabled(!m_redoStack.isEmpty());
        }
    }

    void pushUndoState()
    {
        if (m_undoStack.size() >= k_maxUndoDepth) {
            m_undoStack.removeFirst();
        }
        m_undoStack.push_back(m_model);
        m_redoStack.clear();
        refreshUndoRedoButtons();
    }

    void undo()
    {
        if (m_undoStack.isEmpty()) {
            return;
        }
        m_redoStack.push_back(m_model);
        const CommandTabModel previousState = m_undoStack.takeLast();
        setModel(previousState);
        refreshUndoRedoButtons();
    }

    void redo()
    {
        if (m_redoStack.isEmpty()) {
            return;
        }
        m_undoStack.push_back(m_model);
        const CommandTabModel nextState = m_redoStack.takeLast();
        setModel(nextState);
        refreshUndoRedoButtons();
    }

    void rebuildCommandCatalog()
    {
        m_commandCatalog.clear();

        auto registerCommand = [this](const CommandTabCommandEntry& command) {
            if (
                (command.type != QStringLiteral("command") && command.type != QStringLiteral("dropdown"))
                || command.id.isEmpty()
            ) {
                return;
            }
            if (!m_commandCatalog.contains(command.id)) {
                m_commandCatalog.insert(command.id, command);
            }
        };

        for (const auto& command : m_model.quickAccess) {
            registerCommand(command);
        }
        for (const auto& workbench : m_model.workbenches) {
            for (const auto& panel : workbench.panels) {
                for (const auto& command : panel.commands) {
                    registerCommand(command);
                }
            }
        }

        QStringList commandIds = m_commandCatalog.keys();
        std::sort(commandIds.begin(), commandIds.end(), [this](const QString& left, const QString& right) {
            return commandtabCommandDisplayText(m_commandCatalog.value(left)).compare(
                       commandtabCommandDisplayText(m_commandCatalog.value(right)),
                       Qt::CaseInsensitive
                   )
                < 0;
        });

        auto populateCatalog = [this, &commandIds](QComboBox* comboBox) {
            if (comboBox == nullptr) {
                return;
            }
            comboBox->clear();
            for (const auto& commandId : commandIds) {
                const auto command = m_commandCatalog.value(commandId);
                comboBox->addItem(commandtabCommandDisplayText(command), commandId);
            }
        };

        populateCatalog(m_panelCommandCatalog);
        populateCatalog(m_quickCommandCatalog);
        populateCatalog(m_dropdownCommandCatalog);
    }

    const CommandTabWorkbenchEntry* workbenchById(const QString& workbenchId) const
    {
        if (workbenchId.isEmpty()) {
            return nullptr;
        }
        for (const auto& workbench : m_model.workbenches) {
            if (workbench.id == workbenchId) {
                return &workbench;
            }
        }
        return nullptr;
    }

    const CommandTabWorkbenchVisibilityEntry* workbenchVisibilityById(const QString& workbenchId) const
    {
        if (workbenchId.isEmpty()) {
            return nullptr;
        }
        for (const auto& workbench : m_model.workbenchVisibility) {
            if (workbench.id == workbenchId) {
                return &workbench;
            }
        }
        return nullptr;
    }

    QIcon workbenchIconForId(const QString& workbenchId) const
    {
        const auto* workbench = workbenchById(workbenchId);
        if (workbench != nullptr && !workbench->iconPath.trimmed().isEmpty()) {
            QIcon icon = loadIconFromSource(workbench->iconPath.trimmed());
            if (!icon.isNull()) {
                return icon;
            }
        }

        const auto* visibilityEntry = workbenchVisibilityById(workbenchId);
        if (visibilityEntry != nullptr && !visibilityEntry->iconPath.trimmed().isEmpty()) {
            QIcon icon = loadIconFromSource(visibilityEntry->iconPath.trimmed());
            if (!icon.isNull()) {
                return icon;
            }
        }

        const QString titleFromWorkbench = workbench != nullptr ? workbench->title : QString();
        const QString titleFromVisibility = visibilityEntry != nullptr ? visibilityEntry->title : QString();
        const QString candidateTitle = !titleFromWorkbench.isEmpty() ? titleFromWorkbench : titleFromVisibility;
        for (const auto& candidate : workbenchIconCandidates(workbenchId, candidateTitle)) {
            QIcon icon = loadIconFromSource(candidate);
            if (!icon.isNull()) {
                return icon;
            }
        }
        return QIcon();
    }

    QIcon commandIconForId(const QString& commandId) const
    {
        if (commandId.isEmpty()) {
            return QIcon();
        }
        const auto it = m_commandCatalog.constFind(commandId);
        if (it == m_commandCatalog.constEnd()) {
            return QIcon();
        }
        return loadCommandEntryIcon(it.value());
    }

    QIcon panelIconForEntry(const CommandTabPanelEntry& panel) const
    {
        for (const auto& command : panel.commands) {
            const QIcon icon = loadCommandEntryIcon(command);
            if (!icon.isNull()) {
                return icon;
            }
        }
        return QIcon();
    }

    QIcon panelIconForVisibility(const QString& workbenchId, const QString& panelId) const
    {
        const auto* workbench = workbenchById(workbenchId);
        if (workbench != nullptr) {
            for (const auto& panel : workbench->panels) {
                if (panel.id != panelId) {
                    continue;
                }
                const QIcon panelIcon = panelIconForEntry(panel);
                if (!panelIcon.isNull()) {
                    return panelIcon;
                }
                break;
            }
        }
        return workbenchIconForId(workbenchId);
    }

    void populateWorkbenchList(const QString& selectedWorkbenchId)
    {
        m_workbenchList->clear();
        QString fallbackWorkbenchId;
        for (const auto& workbench : m_model.workbenches) {
            if (workbench.panels.isEmpty()) {
                continue;
            }
            auto* item = new QListWidgetItem(workbench.title.isEmpty() ? workbench.id : workbench.title, m_workbenchList);
            item->setData(Qt::UserRole, workbench.id);
            const QIcon icon = workbenchIconForId(workbench.id);
            if (!icon.isNull()) {
                item->setIcon(icon);
            }
            if (fallbackWorkbenchId.isEmpty()) {
                fallbackWorkbenchId = workbench.id;
            }
        }

        const QString targetWorkbench = selectedWorkbenchId.isEmpty()
            ? (m_model.activeWorkbenchId.isEmpty() ? fallbackWorkbenchId : m_model.activeWorkbenchId)
            : selectedWorkbenchId;
        selectListItemById(m_workbenchList, targetWorkbench);
    }

    void populatePanelList(const QString& selectedPanelId)
    {
        m_updatingUi = true;
        m_panelList->clear();

        const auto* workbench = currentWorkbench();
        QString fallbackPanelId;
        if (workbench != nullptr) {
            for (const auto& panel : workbench->panels) {
                auto* item = new QListWidgetItem(panel.title.isEmpty() ? panel.id : panel.title, m_panelList);
                item->setData(Qt::UserRole, panel.id);
                const QIcon panelIcon = panelIconForEntry(panel);
                if (!panelIcon.isNull()) {
                    item->setIcon(panelIcon);
                }
                if (fallbackPanelId.isEmpty()) {
                    fallbackPanelId = panel.id;
                }
            }
        }

        selectListItemById(m_panelList, selectedPanelId.isEmpty() ? fallbackPanelId : selectedPanelId);
        m_updatingUi = false;
        refreshPanelEditor();
    }

    void populateCommandList(const QString& selectedCommandId)
    {
        m_updatingUi = true;
        m_commandList->clear();

        const auto* panel = currentPanel();
        QString fallbackCommandId;
        if (panel != nullptr) {
            for (const auto& command : panel->commands) {
                const QString label = command.type == QStringLiteral("separator")
                    ? QCoreApplication::translate("CommandTabCustomizationDialog", "Separator")
                    : QStringLiteral("%1  [%2]").arg(commandtabCommandDisplayText(command), command.size.toUpper());
                auto* item = new QListWidgetItem(label, m_commandList);
                item->setData(Qt::UserRole, command.id);
                item->setData(Qt::UserRole + 1, command.type);
                if (command.type != QStringLiteral("separator")) {
                    const QIcon icon = loadCommandEntryIcon(command);
                    if (!icon.isNull()) {
                        item->setIcon(icon);
                    }
                }
                if (fallbackCommandId.isEmpty()) {
                    fallbackCommandId = command.id;
                }
            }
        }

        selectListItemById(m_commandList, selectedCommandId.isEmpty() ? fallbackCommandId : selectedCommandId);
        m_updatingUi = false;
        refreshCommandEditor();
    }

    void populateQuickAccessList(const QString& selectedCommandId)
    {
        m_quickAccessList->clear();
        QString fallbackCommandId;
        for (const auto& command : m_model.quickAccess) {
            auto* item = new QListWidgetItem(commandtabCommandDisplayText(command), m_quickAccessList);
            item->setData(Qt::UserRole, command.id);
            const QIcon icon = loadCommandEntryIcon(command);
            if (!icon.isNull()) {
                item->setIcon(icon);
            }
            if (fallbackCommandId.isEmpty()) {
                fallbackCommandId = command.id;
            }
        }
        selectListItemById(m_quickAccessList, selectedCommandId.isEmpty() ? fallbackCommandId : selectedCommandId);
    }

    void populateWorkbenchVisibilityList()
    {
        const QString selectedWorkbenchId = currentWorkbenchVisibilityId();
        const QSignalBlocker blocker(m_workbenchVisibilityList);
        m_workbenchVisibilityList->clear();
        for (const auto& workbench : m_model.workbenchVisibility) {
            auto* item = new QListWidgetItem(
                workbench.title.isEmpty() ? workbench.id : workbench.title,
                m_workbenchVisibilityList
            );
            item->setData(Qt::UserRole, workbench.id);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(workbench.visible ? Qt::Checked : Qt::Unchecked);
            const QIcon icon = workbenchIconForId(workbench.id);
            if (!icon.isNull()) {
                item->setIcon(icon);
            }
        }
        selectListItemById(m_workbenchVisibilityList, selectedWorkbenchId);
    }

    void populatePanelVisibilityList()
    {
        const QString selectedPanelKey = currentPanelVisibilityKey();
        const QSignalBlocker blocker(m_panelVisibilityList);
        m_panelVisibilityList->clear();
        for (const auto& panel : m_model.panelVisibility) {
            const QString label = QStringLiteral("%1 / %2").arg(
                panel.workbenchTitle.isEmpty() ? panel.workbenchId : panel.workbenchTitle,
                panel.title.isEmpty() ? panel.id : panel.title
            );
            auto* item = new QListWidgetItem(label, m_panelVisibilityList);
            item->setData(Qt::UserRole, panel.id);
            item->setData(Qt::UserRole + 1, panel.workbenchId);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(panel.visible ? Qt::Checked : Qt::Unchecked);
            const QIcon icon = panelIconForVisibility(panel.workbenchId, panel.id);
            if (!icon.isNull()) {
                item->setIcon(icon);
            }
        }
        selectPanelVisibilityByKey(selectedPanelKey);
    }

    void populateDropdownList(const QString& selectedDropdownId)
    {
        m_dropdownList->clear();
        QString fallbackDropdownId;
        for (const auto& dropdown : m_model.dropdownDefinitions) {
            auto* item = new QListWidgetItem(dropdown.text.isEmpty() ? dropdown.id : dropdown.text, m_dropdownList);
            item->setData(Qt::UserRole, dropdown.id);
            const QIcon icon = commandIconForId(dropdown.id);
            if (!icon.isNull()) {
                item->setIcon(icon);
            }
            if (fallbackDropdownId.isEmpty()) {
                fallbackDropdownId = dropdown.id;
            }
        }
        selectListItemById(m_dropdownList, selectedDropdownId.isEmpty() ? fallbackDropdownId : selectedDropdownId);
    }

    void populateDropdownCommandList(const QString& selectedCommandId)
    {
        m_dropdownCommandList->clear();
        const auto* dropdown = currentDropdown();
        QString fallbackCommandId;
        if (dropdown != nullptr) {
            for (const auto& command : dropdown->commands) {
                auto* item = new QListWidgetItem(commandtabCommandDisplayText(command), m_dropdownCommandList);
                item->setData(Qt::UserRole, command.id);
                const QIcon icon = loadCommandEntryIcon(command);
                if (!icon.isNull()) {
                    item->setIcon(icon);
                }
                if (fallbackCommandId.isEmpty()) {
                    fallbackCommandId = command.id;
                }
            }
        }
        selectListItemById(m_dropdownCommandList, selectedCommandId.isEmpty() ? fallbackCommandId : selectedCommandId);
    }

    void refreshPanelEditor()
    {
        const auto* panel = currentPanel();
        const bool hasPanel = panel != nullptr;
        m_panelTitleEdit->setEnabled(hasPanel);
        m_panelTitleEdit->setText(hasPanel ? panel->title : QString());
        m_panelSourceTypeCombo->setEnabled(hasPanel);
        m_panelSourceWorkbenchCombo->setEnabled(hasPanel);
        {
            const QSignalBlocker typeBlocker(m_panelSourceTypeCombo);
            const QSignalBlocker scopeBlocker(m_panelSourceWorkbenchCombo);
            m_panelSourceWorkbenchCombo->clear();
            if (!hasPanel) {
                m_panelSourceTypeCombo->setCurrentIndex(0);
                return;
            }

            const QString sourceType = panel->sourceType.isEmpty() ? QStringLiteral("toolbar") : panel->sourceType;
            const int typeIndex = m_panelSourceTypeCombo->findData(sourceType);
            m_panelSourceTypeCombo->setCurrentIndex(typeIndex >= 0 ? typeIndex : 0);

            auto addWorkbenchOption = [this](const QString& workbenchId, const QString& title) {
                if (workbenchId.isEmpty()) {
                    return;
                }
                if (m_panelSourceWorkbenchCombo->findData(workbenchId) >= 0) {
                    return;
                }
                m_panelSourceWorkbenchCombo->addItem(title.isEmpty() ? workbenchId : title, workbenchId);
            };

            const QString globalLabel = QCoreApplication::translate("CommandTabCustomizationDialog", "Global");
            addWorkbenchOption(QStringLiteral("Global"), globalLabel);
            for (const auto& workbench : m_model.workbenchVisibility) {
                addWorkbenchOption(workbench.id, workbench.title);
            }
            for (const auto& workbench : m_model.workbenches) {
                addWorkbenchOption(workbench.id, workbench.title);
            }
            addWorkbenchOption(currentWorkbenchId(), currentWorkbenchId());

            const QString scopeId = panel->sourceWorkbenchId.isEmpty() ? currentWorkbenchId() : panel->sourceWorkbenchId;
            const int scopeIndex = m_panelSourceWorkbenchCombo->findData(scopeId);
            m_panelSourceWorkbenchCombo->setCurrentIndex(scopeIndex >= 0 ? scopeIndex : 0);
            const bool scopeEditable = sourceType == QStringLiteral("new");
            m_panelSourceWorkbenchCombo->setEnabled(scopeEditable);
        }
    }

    void refreshCommandEditor()
    {
        const auto* command = currentCommand();
        const bool hasEditableCommand = command != nullptr
            && (command->type == QStringLiteral("command") || command->type == QStringLiteral("dropdown"));
        m_commandTextEdit->setEnabled(hasEditableCommand);
        m_commandSizeCombo->setEnabled(hasEditableCommand);
        m_commandSourceWorkbenchCombo->setEnabled(hasEditableCommand);
        m_commandSourceToolbarEdit->setEnabled(hasEditableCommand);
        if (!hasEditableCommand) {
            m_commandTextEdit->clear();
            m_commandSizeCombo->setCurrentIndex(0);
            m_commandSourceWorkbenchCombo->clear();
            m_commandSourceToolbarEdit->clear();
            return;
        }

        {
            const QSignalBlocker sourceWorkbenchBlocker(m_commandSourceWorkbenchCombo);
            m_commandTextEdit->setText(commandtabCommandDisplayText(*command));
            const QString size = command->size.isEmpty() ? QStringLiteral("small") : command->size;
            const int index = m_commandSizeCombo->findData(size);
            m_commandSizeCombo->setCurrentIndex(index >= 0 ? index : 0);

            m_commandSourceWorkbenchCombo->clear();
            auto addWorkbenchOption = [this](const QString& workbenchId, const QString& title) {
                if (workbenchId.isEmpty()) {
                    return;
                }
                if (m_commandSourceWorkbenchCombo->findData(workbenchId) >= 0) {
                    return;
                }
                m_commandSourceWorkbenchCombo->addItem(title.isEmpty() ? workbenchId : title, workbenchId);
            };

            addWorkbenchOption(
                QStringLiteral("General"),
                QCoreApplication::translate("CommandTabCustomizationDialog", "General")
            );
            addWorkbenchOption(
                QStringLiteral("Standard"),
                QCoreApplication::translate("CommandTabCustomizationDialog", "Standard")
            );
            addWorkbenchOption(
                QStringLiteral("Global"),
                QCoreApplication::translate("CommandTabCustomizationDialog", "Global")
            );
            for (const auto& workbench : m_model.workbenchVisibility) {
                addWorkbenchOption(workbench.id, workbench.title);
            }
            for (const auto& workbench : m_model.workbenches) {
                addWorkbenchOption(workbench.id, workbench.title);
            }
            addWorkbenchOption(currentWorkbenchId(), currentWorkbenchId());

            const QString sourceWorkbenchId = command->sourceWorkbenchId.isEmpty()
                ? currentWorkbenchId()
                : command->sourceWorkbenchId;
            const int sourceWorkbenchIndex = m_commandSourceWorkbenchCombo->findData(sourceWorkbenchId);
            m_commandSourceWorkbenchCombo->setCurrentIndex(sourceWorkbenchIndex >= 0 ? sourceWorkbenchIndex : 0);
        }
        m_commandSourceToolbarEdit->setText(command->sourceToolbarTitle);

        const auto* panel = currentPanel();
        const QString panelSourceType = panel == nullptr ? QString() : panel->sourceType;
        m_commandSourceWorkbenchCombo->setEnabled(
            hasEditableCommand
            && (panelSourceType == QStringLiteral("new") || command->type == QStringLiteral("dropdown"))
        );
        m_commandSourceToolbarEdit->setEnabled(
            hasEditableCommand
            && panelSourceType == QStringLiteral("custom")
        );
    }

    void refreshQuickAccessButtons()
    {
        if (m_quickAccessList->count() > 0 && m_quickAccessList->currentRow() < 0) {
            m_quickAccessList->setCurrentRow(0);
        }
    }

    void refreshWorkbenchVisibilityButtons()
    {
        if (m_workbenchVisibilityList->count() > 0 && m_workbenchVisibilityList->currentRow() < 0) {
            m_workbenchVisibilityList->setCurrentRow(0);
        }
    }

    void refreshPanelVisibilityButtons()
    {
        if (m_panelVisibilityList->count() > 0 && m_panelVisibilityList->currentRow() < 0) {
            m_panelVisibilityList->setCurrentRow(0);
        }
    }

    void refreshDropdownEditor()
    {
        const auto* dropdown = currentDropdown();
        const bool hasDropdown = dropdown != nullptr;
        m_dropdownNameEdit->setEnabled(hasDropdown);
        m_dropdownCommandCatalog->setEnabled(hasDropdown);
        m_dropdownCommandList->setEnabled(hasDropdown);
        if (!hasDropdown) {
            m_dropdownNameEdit->clear();
            return;
        }
        m_dropdownNameEdit->setText(dropdown->text.isEmpty() ? dropdown->id : dropdown->text);
    }

    void toggleSelectedWorkbenchVisibility()
    {
        auto* item = m_workbenchVisibilityList->currentItem();
        if (item == nullptr) {
            return;
        }
        item->setCheckState(item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
    }

    void toggleSelectedPanelVisibility()
    {
        auto* item = m_panelVisibilityList->currentItem();
        if (item == nullptr) {
            return;
        }
        item->setCheckState(item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
    }

    void applyWorkbenchVisibilityItemState(QListWidgetItem* item)
    {
        if (m_updatingUi || item == nullptr) {
            return;
        }

        const QString workbenchId = item->data(Qt::UserRole).toString();
        if (workbenchId.isEmpty()) {
            return;
        }
        pushUndoState();
        const bool isVisible = item->checkState() == Qt::Checked;
        for (auto& workbench : m_model.workbenchVisibility) {
            if (workbench.id == workbenchId) {
                workbench.visible = isVisible;
                break;
            }
        }
    }

    void applyPanelVisibilityItemState(QListWidgetItem* item)
    {
        if (m_updatingUi || item == nullptr) {
            return;
        }

        const QString panelId = item->data(Qt::UserRole).toString();
        const QString workbenchId = item->data(Qt::UserRole + 1).toString();
        if (panelId.isEmpty() || workbenchId.isEmpty()) {
            return;
        }
        pushUndoState();
        const bool isVisible = item->checkState() == Qt::Checked;
        for (auto& panel : m_model.panelVisibility) {
            if (panel.id == panelId && panel.workbenchId == workbenchId) {
                panel.visible = isVisible;
                break;
            }
        }
    }

    void setAllWorkbenchVisibility(bool visible)
    {
        if (m_model.workbenchVisibility.isEmpty()) {
            return;
        }
        pushUndoState();
        for (auto& workbench : m_model.workbenchVisibility) {
            workbench.visible = visible;
        }
        populateWorkbenchVisibilityList();
    }

    void setAllPanelVisibility(bool visible)
    {
        if (m_model.panelVisibility.isEmpty()) {
            return;
        }
        pushUndoState();
        for (auto& panel : m_model.panelVisibility) {
            panel.visible = visible;
        }
        populatePanelVisibilityList();
    }

    void createDropdown()
    {
        pushUndoState();
        CommandTabDropdownDefinitionEntry dropdown;
        const QString customDropdownText =
            QCoreApplication::translate("CommandTabCustomizationDialog", "Custom Dropdown");
        dropdown.id = uniqueDropdownId(customDropdownText);
        dropdown.text = customDropdownText;
        m_model.dropdownDefinitions.push_back(dropdown);
        CommandTabCommandEntry catalogEntry;
        catalogEntry.type = QStringLiteral("dropdown");
        catalogEntry.id = dropdown.id;
        catalogEntry.text = dropdown.text;
        catalogEntry.size = QStringLiteral("small");
        m_commandCatalog.insert(catalogEntry.id, catalogEntry);
        populateDropdownList(dropdown.id);
        populateDropdownCommandList(QString());
        refreshDropdownEditor();
    }

    void removeDropdown()
    {
        const QString dropdownId = currentDropdownId();
        if (dropdownId.isEmpty()) {
            return;
        }
        pushUndoState();
        for (int index = 0; index < m_model.dropdownDefinitions.size(); ++index) {
            if (m_model.dropdownDefinitions.at(index).id == dropdownId) {
                m_model.dropdownDefinitions.removeAt(index);
                break;
            }
        }
        m_commandCatalog.remove(dropdownId);
        removeDropdownUsages(dropdownId);
        populateDropdownList(QString());
        populateDropdownCommandList(QString());
        refreshDropdownEditor();
    }

    void applyDropdownNameEdit()
    {
        if (m_updatingUi) {
            return;
        }
        auto* dropdown = currentDropdownMutable();
        if (dropdown == nullptr) {
            return;
        }
        const QString nextText = m_dropdownNameEdit->text().simplified();
        if (nextText == dropdown->text) {
            return;
        }
        pushUndoState();
        dropdown->text = nextText;
        if (m_commandCatalog.contains(dropdown->id)) {
            auto command = m_commandCatalog.value(dropdown->id);
            command.text = nextText;
            command.menuCommands = dropdown->commands;
            m_commandCatalog.insert(dropdown->id, command);
        }
        syncDropdownUsage(dropdown->id, nextText, dropdown->commands);
        populateDropdownList(dropdown->id);
    }

    void addCatalogCommandToDropdown()
    {
        auto* dropdown = currentDropdownMutable();
        if (dropdown == nullptr) {
            return;
        }
        const QString commandId = m_dropdownCommandCatalog->currentData().toString();
        if (commandId.isEmpty() || commandId == dropdown->id) {
            return;
        }
        CommandTabCommandEntry command = m_commandCatalog.value(commandId);
        if (command.id.isEmpty()) {
            return;
        }
        pushUndoState();
        if (command.sourceWorkbenchId.isEmpty()) {
            command.sourceWorkbenchId = currentWorkbenchId();
        }
        dropdown->commands.push_back(command);
        syncDropdownUsage(dropdown->id, dropdown->text, dropdown->commands);
        populateDropdownCommandList(command.id);
    }

    void moveSelectedDropdownCommand(int delta)
    {
        auto* dropdown = currentDropdownMutable();
        if (dropdown == nullptr) {
            return;
        }
        const int row = m_dropdownCommandList->currentRow();
        const int targetRow = row + delta;
        if (row < 0 || targetRow < 0 || targetRow >= dropdown->commands.size()) {
            return;
        }
        pushUndoState();
        dropdown->commands.swapItemsAt(row, targetRow);
        syncDropdownUsage(dropdown->id, dropdown->text, dropdown->commands);
        populateDropdownCommandList(dropdown->commands.at(targetRow).id);
    }

    void removeSelectedDropdownCommand()
    {
        auto* dropdown = currentDropdownMutable();
        if (dropdown == nullptr) {
            return;
        }
        const int row = m_dropdownCommandList->currentRow();
        if (row < 0 || row >= dropdown->commands.size()) {
            return;
        }
        pushUndoState();
        dropdown->commands.removeAt(row);
        syncDropdownUsage(dropdown->id, dropdown->text, dropdown->commands);
        populateDropdownCommandList(QString());
    }

    void moveSelectedPanel(int delta)
    {
        auto* workbench = currentWorkbenchMutable();
        if (workbench == nullptr) {
            return;
        }
        const int row = m_panelList->currentRow();
        const int targetRow = row + delta;
        if (row < 0 || targetRow < 0 || targetRow >= workbench->panels.size()) {
            return;
        }
        pushUndoState();
        workbench->panels.swapItemsAt(row, targetRow);
        populatePanelList(workbench->panels.at(targetRow).id);
        populateCommandList(QString());
    }

    void hideSelectedPanel()
    {
        auto* workbench = currentWorkbenchMutable();
        if (workbench == nullptr) {
            return;
        }
        const int row = m_panelList->currentRow();
        if (row < 0 || row >= workbench->panels.size()) {
            return;
        }
        pushUndoState();
        workbench->panels.removeAt(row);
        populatePanelList(QString());
        populateCommandList(QString());
    }

    void createPanel()
    {
        auto* workbench = currentWorkbenchMutable();
        if (workbench == nullptr) {
            return;
        }
        pushUndoState();
        CommandTabPanelEntry panel;
        panel.title = nextPanelTitle();
        panel.id = uniquePanelId(*workbench, panel.title, QStringLiteral("custom_panel"));
        panel.sourceType = QStringLiteral("new");
        panel.sourceWorkbenchId = currentWorkbenchId();
        workbench->panels.push_back(panel);
        populatePanelList(panel.id);
        populateCommandList(QString());
    }

    void duplicatePanel()
    {
        auto* workbench = currentWorkbenchMutable();
        const auto* sourcePanel = currentPanel();
        if (workbench == nullptr || sourcePanel == nullptr) {
            return;
        }
        pushUndoState();
        CommandTabPanelEntry panel = *sourcePanel;
        panel.title = QCoreApplication::translate("CommandTabCustomizationDialog", "%1 Copy")
            .arg(sourcePanel->title.isEmpty() ? sourcePanel->id : sourcePanel->title);
        panel.id = uniquePanelId(*workbench, panel.title, QStringLiteral("%1_copy").arg(sourcePanel->id));
        panel.sourceType = QStringLiteral("new");
        panel.sourceWorkbenchId = currentWorkbenchId();
        workbench->panels.push_back(panel);
        populatePanelList(panel.id);
        populateCommandList(QString());
    }

    void moveSelectedCommand(int delta)
    {
        auto* panel = currentPanelMutable();
        if (panel == nullptr) {
            return;
        }
        const int row = m_commandList->currentRow();
        const int targetRow = row + delta;
        if (row < 0 || targetRow < 0 || targetRow >= panel->commands.size()) {
            return;
        }
        pushUndoState();
        panel->commands.swapItemsAt(row, targetRow);
        populateCommandList(panel->commands.at(targetRow).id);
    }

    void removeSelectedCommand()
    {
        auto* panel = currentPanelMutable();
        if (panel == nullptr) {
            return;
        }
        const int row = m_commandList->currentRow();
        if (row < 0 || row >= panel->commands.size()) {
            return;
        }
        pushUndoState();
        panel->commands.removeAt(row);
        populateCommandList(QString());
    }

    void addSeparatorToPanel()
    {
        auto* panel = currentPanelMutable();
        const auto* workbench = currentWorkbench();
        if (panel == nullptr || workbench == nullptr) {
            return;
        }
        pushUndoState();
        CommandTabCommandEntry separator;
        separator.type = QStringLiteral("separator");
        separator.id = QStringLiteral("native_%1_separator_%2_%3")
                           .arg(workbench->id, panel->id)
                           .arg(panel->commands.size() + 1);
        separator.size = QStringLiteral("small");
        panel->commands.push_back(separator);
        populateCommandList(separator.id);
    }

    void addCatalogCommandToPanel()
    {
        auto* panel = currentPanelMutable();
        if (panel == nullptr) {
            return;
        }

        const QString commandId = m_panelCommandCatalog->currentData().toString();
        if (commandId.isEmpty()) {
            return;
        }
        for (const auto& existingCommand : panel->commands) {
            if (
                existingCommand.id == commandId
                && (existingCommand.type == QStringLiteral("command")
                    || existingCommand.type == QStringLiteral("dropdown"))
            ) {
                populateCommandList(commandId);
                return;
            }
        }
        pushUndoState();
        CommandTabCommandEntry command = m_commandCatalog.value(commandId);
        if (command.type.isEmpty()) {
            command.type = QStringLiteral("command");
        }
        if (command.size.isEmpty()) {
            command.size = QStringLiteral("small");
        }
        if (panel->sourceType == QStringLiteral("new")) {
            if (command.sourceWorkbenchId.isEmpty()) {
                command.sourceWorkbenchId = panel->sourceWorkbenchId.isEmpty()
                    ? currentWorkbenchId()
                    : panel->sourceWorkbenchId;
            }
        } else if (panel->sourceType == QStringLiteral("custom")) {
            if (command.sourceWorkbenchId.isEmpty()) {
                command.sourceWorkbenchId = currentWorkbenchId();
            }
            if (command.sourceToolbarTitle.isEmpty()) {
                command.sourceToolbarTitle = panel->title;
            }
        } else {
            if (command.sourceWorkbenchId.isEmpty()) {
                command.sourceWorkbenchId = currentWorkbenchId();
            }
            if (command.sourceToolbarTitle.isEmpty()) {
                command.sourceToolbarTitle = panel->title;
            }
        }
        panel->commands.push_back(command);
        populateCommandList(commandId);
    }

    void moveQuickAccessCommand(int delta)
    {
        const int row = m_quickAccessList->currentRow();
        const int targetRow = row + delta;
        if (row < 0 || targetRow < 0 || targetRow >= m_model.quickAccess.size()) {
            return;
        }
        pushUndoState();
        m_model.quickAccess.swapItemsAt(row, targetRow);
        populateQuickAccessList(m_model.quickAccess.at(targetRow).id);
    }

    void removeQuickAccessCommand()
    {
        const int row = m_quickAccessList->currentRow();
        if (row < 0 || row >= m_model.quickAccess.size()) {
            return;
        }
        pushUndoState();
        m_model.quickAccess.removeAt(row);
        populateQuickAccessList(QString());
    }

    void addCatalogCommandToQuickAccess()
    {
        const QString commandId = m_quickCommandCatalog->currentData().toString();
        if (commandId.isEmpty()) {
            return;
        }
        for (const auto& command : m_model.quickAccess) {
            if (
                command.id == commandId
                && (command.type == QStringLiteral("command")
                    || command.type == QStringLiteral("dropdown"))
            ) {
                populateQuickAccessList(commandId);
                return;
            }
        }
        pushUndoState();
        CommandTabCommandEntry command = m_commandCatalog.value(commandId);
        if (command.type.isEmpty()) {
            command.type = QStringLiteral("command");
        }
        command.size = QStringLiteral("small");
        m_model.quickAccess.push_back(command);
        populateQuickAccessList(commandId);
    }

    void applyPanelTitleEdit()
    {
        if (m_updatingUi) {
            return;
        }
        auto* panel = currentPanelMutable();
        if (panel == nullptr) {
            return;
        }
        const QString nextTitle = m_panelTitleEdit->text().simplified();
        if (nextTitle == panel->title) {
            return;
        }
        pushUndoState();
        panel->title = nextTitle;
        populatePanelList(panel->id);
    }

    void applyPanelSourceTypeEdit()
    {
        if (m_updatingUi) {
            return;
        }
        auto* panel = currentPanelMutable();
        if (panel == nullptr) {
            return;
        }
        const QString sourceType = m_panelSourceTypeCombo->currentData().toString().trimmed().toLower();
        if (sourceType.isEmpty() || sourceType == panel->sourceType) {
            return;
        }
        pushUndoState();
        panel->sourceType = sourceType;
        if (sourceType == QStringLiteral("new")) {
            if (panel->sourceWorkbenchId.isEmpty()) {
                panel->sourceWorkbenchId = currentWorkbenchId();
            }
        } else {
            panel->sourceWorkbenchId = currentWorkbenchId();
        }
        refreshPanelEditor();
    }

    void applyPanelSourceWorkbenchEdit()
    {
        if (m_updatingUi) {
            return;
        }
        auto* panel = currentPanelMutable();
        if (panel == nullptr || panel->sourceType != QStringLiteral("new")) {
            return;
        }
        const QString sourceWorkbenchId = m_panelSourceWorkbenchCombo->currentData().toString().trimmed();
        if (sourceWorkbenchId.isEmpty() || sourceWorkbenchId == panel->sourceWorkbenchId) {
            return;
        }
        pushUndoState();
        panel->sourceWorkbenchId = sourceWorkbenchId;
        refreshPanelEditor();
    }

    void applyCommandTextEdit()
    {
        if (m_updatingUi) {
            return;
        }
        auto* command = currentCommandMutable();
        if (command == nullptr || command->type != QStringLiteral("command")) {
            return;
        }
        const QString nextText = m_commandTextEdit->text().simplified();
        if (nextText == command->text) {
            return;
        }
        pushUndoState();
        command->text = nextText;
        populateCommandList(command->id);
    }

    void applyCommandSizeEdit()
    {
        if (m_updatingUi) {
            return;
        }
        auto* command = currentCommandMutable();
        if (command == nullptr || command->type != QStringLiteral("command")) {
            return;
        }
        const QString size = m_commandSizeCombo->currentData().toString();
        const QString nextSize = size.isEmpty() ? QStringLiteral("small") : size;
        if (nextSize == command->size) {
            return;
        }
        pushUndoState();
        command->size = nextSize;
        populateCommandList(command->id);
    }

    void applyCommandSourceWorkbenchEdit()
    {
        if (m_updatingUi) {
            return;
        }
        auto* command = currentCommandMutable();
        if (command == nullptr) {
            return;
        }
        const QString sourceWorkbenchId = m_commandSourceWorkbenchCombo->currentData().toString().trimmed();
        if (sourceWorkbenchId.isEmpty() || sourceWorkbenchId == command->sourceWorkbenchId) {
            return;
        }
        pushUndoState();
        command->sourceWorkbenchId = sourceWorkbenchId;
        refreshCommandEditor();
    }

    void applyCommandSourceToolbarEdit()
    {
        if (m_updatingUi) {
            return;
        }
        auto* command = currentCommandMutable();
        if (command == nullptr) {
            return;
        }
        const QString nextToolbar = m_commandSourceToolbarEdit->text().simplified();
        if (nextToolbar == command->sourceToolbarTitle) {
            return;
        }
        pushUndoState();
        command->sourceToolbarTitle = nextToolbar;
        refreshCommandEditor();
    }

    void submitCustomization(const QString& commandPrefix)
    {
        if (!m_commandHandler) {
            return;
        }
        const QByteArray encodedPayload = encodeCustomizationPayload(m_model);
        m_commandHandler(QStringLiteral("%1:%2").arg(commandPrefix, QString::fromLatin1(encodedPayload)));
    }

    void exportLayout()
    {
        if (!m_commandHandler) {
            return;
        }
        const QString path = QFileDialog::getSaveFileName(
            this,
            QCoreApplication::translate("CommandTabCustomizationDialog", "Export commandtab layout"),
            QDir::homePath() + QStringLiteral("/CommandTabStructure.json"),
            QCoreApplication::translate("CommandTabCustomizationDialog", "CommandTab Structure (*.json)")
        );
        if (path.isEmpty()) {
            return;
        }
        m_commandHandler(QStringLiteral("__commandtab_design_export__:%1").arg(encodeCommandTabCommandArgument(path)));
    }

    void importLayout()
    {
        if (!m_commandHandler) {
            return;
        }
        const QString path = QFileDialog::getOpenFileName(
            this,
            QCoreApplication::translate("CommandTabCustomizationDialog", "Import commandtab layout"),
            QDir::homePath(),
            QCoreApplication::translate("CommandTabCustomizationDialog", "CommandTab Structure (*.json)")
        );
        if (path.isEmpty()) {
            return;
        }
        m_commandHandler(QStringLiteral("__commandtab_design_import__:%1").arg(encodeCommandTabCommandArgument(path)));
    }

    void triggerSimpleCommand(const QString& command)
    {
        if (m_commandHandler) {
            m_commandHandler(command);
        }
    }

    const CommandTabDropdownDefinitionEntry* currentDropdown() const
    {
        const QString dropdownId = currentDropdownId();
        if (dropdownId.isEmpty()) {
            return nullptr;
        }
        for (const auto& dropdown : m_model.dropdownDefinitions) {
            if (dropdown.id == dropdownId) {
                return &dropdown;
            }
        }
        return nullptr;
    }

    CommandTabDropdownDefinitionEntry* currentDropdownMutable()
    {
        const QString dropdownId = currentDropdownId();
        if (dropdownId.isEmpty()) {
            return nullptr;
        }
        for (int index = 0; index < m_model.dropdownDefinitions.size(); ++index) {
            if (m_model.dropdownDefinitions.at(index).id == dropdownId) {
                return &m_model.dropdownDefinitions[index];
            }
        }
        return nullptr;
    }

    const CommandTabWorkbenchEntry* currentWorkbench() const
    {
        const QString workbenchId = currentWorkbenchId();
        if (workbenchId.isEmpty()) {
            return nullptr;
        }
        for (const auto& workbench : m_model.workbenches) {
            if (workbench.id == workbenchId) {
                return &workbench;
            }
        }
        return nullptr;
    }

    CommandTabWorkbenchEntry* currentWorkbenchMutable()
    {
        const QString workbenchId = currentWorkbenchId();
        if (workbenchId.isEmpty()) {
            return nullptr;
        }
        for (int index = 0; index < m_model.workbenches.size(); ++index) {
            if (m_model.workbenches.at(index).id == workbenchId) {
                return &m_model.workbenches[index];
            }
        }
        return nullptr;
    }

    const CommandTabPanelEntry* currentPanel() const
    {
        const auto* workbench = currentWorkbench();
        const int row = m_panelList->currentRow();
        if (workbench == nullptr || row < 0 || row >= workbench->panels.size()) {
            return nullptr;
        }
        return &workbench->panels.at(row);
    }

    CommandTabPanelEntry* currentPanelMutable()
    {
        auto* workbench = currentWorkbenchMutable();
        const int row = m_panelList->currentRow();
        if (workbench == nullptr || row < 0 || row >= workbench->panels.size()) {
            return nullptr;
        }
        return &workbench->panels[row];
    }

    const CommandTabCommandEntry* currentCommand() const
    {
        const auto* panel = currentPanel();
        const int row = m_commandList->currentRow();
        if (panel == nullptr || row < 0 || row >= panel->commands.size()) {
            return nullptr;
        }
        return &panel->commands.at(row);
    }

    CommandTabCommandEntry* currentCommandMutable()
    {
        auto* panel = currentPanelMutable();
        const int row = m_commandList->currentRow();
        if (panel == nullptr || row < 0 || row >= panel->commands.size()) {
            return nullptr;
        }
        return &panel->commands[row];
    }

    QString currentWorkbenchId() const
    {
        const auto* item = m_workbenchList->currentItem();
        return item != nullptr ? item->data(Qt::UserRole).toString() : QString();
    }

    QString currentPanelId() const
    {
        const auto* item = m_panelList->currentItem();
        return item != nullptr ? item->data(Qt::UserRole).toString() : QString();
    }

    QString currentCommandId() const
    {
        const auto* item = m_commandList->currentItem();
        return item != nullptr ? item->data(Qt::UserRole).toString() : QString();
    }

    QString currentQuickAccessCommandId() const
    {
        const auto* item = m_quickAccessList->currentItem();
        return item != nullptr ? item->data(Qt::UserRole).toString() : QString();
    }

    QString currentDropdownId() const
    {
        const auto* item = m_dropdownList->currentItem();
        return item != nullptr ? item->data(Qt::UserRole).toString() : QString();
    }

    QString currentWorkbenchVisibilityId() const
    {
        const auto* item = m_workbenchVisibilityList->currentItem();
        return item != nullptr ? item->data(Qt::UserRole).toString() : QString();
    }

    QString currentPanelVisibilityKey() const
    {
        const auto* item = m_panelVisibilityList->currentItem();
        if (item == nullptr) {
            return QString();
        }
        const QString panelId = item->data(Qt::UserRole).toString();
        const QString workbenchId = item->data(Qt::UserRole + 1).toString();
        if (panelId.isEmpty() || workbenchId.isEmpty()) {
            return QString();
        }
        return workbenchId + QStringLiteral("|") + panelId;
    }

    void selectPanelVisibilityByKey(const QString& key)
    {
        if (m_panelVisibilityList == nullptr) {
            return;
        }
        if (key.isEmpty()) {
            if (m_panelVisibilityList->count() > 0) {
                m_panelVisibilityList->setCurrentRow(0);
            }
            return;
        }

        for (int index = 0; index < m_panelVisibilityList->count(); ++index) {
            auto* item = m_panelVisibilityList->item(index);
            if (item == nullptr) {
                continue;
            }
            const QString panelId = item->data(Qt::UserRole).toString();
            const QString workbenchId = item->data(Qt::UserRole + 1).toString();
            if (workbenchId + QStringLiteral("|") + panelId == key) {
                m_panelVisibilityList->setCurrentRow(index);
                return;
            }
        }

        if (m_panelVisibilityList->count() > 0) {
            m_panelVisibilityList->setCurrentRow(0);
        }
    }

    static void selectListItemById(QListWidget* listWidget, const QString& itemId)
    {
        if (listWidget == nullptr) {
            return;
        }
        if (itemId.isEmpty()) {
            if (listWidget->count() > 0) {
                listWidget->setCurrentRow(0);
            }
            return;
        }

        for (int index = 0; index < listWidget->count(); ++index) {
            auto* item = listWidget->item(index);
            if (item != nullptr && item->data(Qt::UserRole).toString() == itemId) {
                listWidget->setCurrentRow(index);
                return;
            }
        }
        if (listWidget->count() > 0) {
            listWidget->setCurrentRow(0);
        }
    }

    static QString sanitizeIdentifier(const QString& value)
    {
        QString result;
        const QString normalized = value.trimmed().toLower();
        bool lastUnderscore = false;
        for (const QChar ch : normalized) {
            if (ch.isLetterOrNumber()) {
                result.append(ch);
                lastUnderscore = false;
                continue;
            }
            if (!lastUnderscore) {
                result.append(QLatin1Char('_'));
                lastUnderscore = true;
            }
        }
        while (result.endsWith(QLatin1Char('_'))) {
            result.chop(1);
        }
        return result;
    }

    static QString uniquePanelId(const CommandTabWorkbenchEntry& workbench, const QString& title, const QString& fallbackPrefix)
    {
        const QString base = sanitizeIdentifier(title).isEmpty() ? fallbackPrefix : sanitizeIdentifier(title);
        QString candidate = base;
        int suffix = 2;
        auto exists = [&workbench](const QString& panelId) {
            for (const auto& panel : workbench.panels) {
                if (panel.id == panelId) {
                    return true;
                }
            }
            return false;
        };
        while (exists(candidate)) {
            candidate = QStringLiteral("%1_%2").arg(base).arg(suffix);
            ++suffix;
        }
        return candidate;
    }

    QString uniqueDropdownId(const QString& title) const
    {
        QString base = sanitizeIdentifier(title);
        if (base.isEmpty()) {
            base = QStringLiteral("custom_dropdown");
        }
        if (!base.endsWith(QStringLiteral("_ddb"))) {
            base += QStringLiteral("_ddb");
        }

        auto exists = [this](const QString& dropdownId) {
            for (const auto& dropdown : m_model.dropdownDefinitions) {
                if (dropdown.id == dropdownId) {
                    return true;
                }
            }
            return false;
        };

        QString candidate = base;
        int suffix = 2;
        while (exists(candidate)) {
            candidate = QStringLiteral("%1_%2").arg(base).arg(suffix);
            ++suffix;
        }
        return candidate;
    }

    void syncDropdownUsage(
        const QString& dropdownId,
        const QString& dropdownText,
        const QVector<CommandTabCommandEntry>& menuCommands
    )
    {
        if (dropdownId.isEmpty()) {
            return;
        }

        auto updateCommand = [&](CommandTabCommandEntry* command) {
            if (command == nullptr || command->id != dropdownId) {
                return;
            }
            command->type = QStringLiteral("dropdown");
            command->text = dropdownText;
            command->menuCommands = menuCommands;
            if (command->size.isEmpty()) {
                command->size = QStringLiteral("small");
            }
        };

        for (auto& command : m_model.quickAccess) {
            updateCommand(&command);
        }
        for (auto& workbench : m_model.workbenches) {
            for (auto& panel : workbench.panels) {
                for (auto& command : panel.commands) {
                    updateCommand(&command);
                }
            }
        }

        CommandTabCommandEntry catalogEntry = m_commandCatalog.value(dropdownId);
        catalogEntry.type = QStringLiteral("dropdown");
        catalogEntry.id = dropdownId;
        catalogEntry.text = dropdownText;
        catalogEntry.size = catalogEntry.size.isEmpty() ? QStringLiteral("small") : catalogEntry.size;
        catalogEntry.menuCommands = menuCommands;
        m_commandCatalog.insert(dropdownId, catalogEntry);
        rebuildCommandCatalog();
    }

    void removeDropdownUsages(const QString& dropdownId)
    {
        if (dropdownId.isEmpty()) {
            return;
        }

        auto shouldRemove = [&dropdownId](const CommandTabCommandEntry& command) {
            return command.id == dropdownId && command.type == QStringLiteral("dropdown");
        };

        for (qsizetype index = m_model.quickAccess.size(); index > 0; --index) {
            const qsizetype candidateIndex = index - 1;
            if (shouldRemove(m_model.quickAccess.at(candidateIndex))) {
                m_model.quickAccess.removeAt(candidateIndex);
            }
        }
        for (auto& workbench : m_model.workbenches) {
            for (auto& panel : workbench.panels) {
                for (qsizetype index = panel.commands.size(); index > 0; --index) {
                    const qsizetype candidateIndex = index - 1;
                    if (shouldRemove(panel.commands.at(candidateIndex))) {
                        panel.commands.removeAt(candidateIndex);
                    }
                }
            }
        }
    }

    QString nextPanelTitle() const
    {
        const auto* workbench = currentWorkbench();
        int counter = 1;
        auto exists = [workbench](const QString& title) {
            if (workbench == nullptr) {
                return false;
            }
            for (const auto& panel : workbench->panels) {
                if (panel.title.compare(title, Qt::CaseInsensitive) == 0) {
                    return true;
                }
            }
            return false;
        };
        QString title = QCoreApplication::translate("CommandTabCustomizationDialog", "Custom Panel");
        while (exists(title)) {
            ++counter;
            title = QCoreApplication::translate("CommandTabCustomizationDialog", "Custom Panel %1").arg(counter);
        }
        return title;
    }

    CommandTabModel m_model;
    CommandTabModel::CommandTabTheme m_theme;
    QVector<CommandTabModel> m_undoStack;
    QVector<CommandTabModel> m_redoStack;
    QPushButton* m_undoButton = nullptr;
    QPushButton* m_redoButton = nullptr;
    QTabWidget* m_tabWidget = nullptr;
    QListWidget* m_workbenchList = nullptr;
    QListWidget* m_panelList = nullptr;
    QListWidget* m_commandList = nullptr;
    QListWidget* m_quickAccessList = nullptr;
    QListWidget* m_workbenchVisibilityList = nullptr;
    QListWidget* m_panelVisibilityList = nullptr;
    QListWidget* m_dropdownList = nullptr;
    QListWidget* m_dropdownCommandList = nullptr;
    QLineEdit* m_panelTitleEdit = nullptr;
    QLineEdit* m_commandTextEdit = nullptr;
    QLineEdit* m_dropdownNameEdit = nullptr;
    QLineEdit* m_commandSourceToolbarEdit = nullptr;
    QComboBox* m_panelSourceTypeCombo = nullptr;
    QComboBox* m_panelSourceWorkbenchCombo = nullptr;
    QComboBox* m_commandSizeCombo = nullptr;
    QComboBox* m_commandSourceWorkbenchCombo = nullptr;
    QComboBox* m_panelCommandCatalog = nullptr;
    QComboBox* m_quickCommandCatalog = nullptr;
    QComboBox* m_dropdownCommandCatalog = nullptr;
    QHash<QString, CommandTabCommandEntry> m_commandCatalog;
    std::function<void(const QString&)> m_commandHandler;
    bool m_updatingUi = false;
};
