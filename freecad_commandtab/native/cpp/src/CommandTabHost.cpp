#include "FreeCADCommandTabNative/Export.h"

#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QDialog>
#include <QDockWidget>
#include <QEnterEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFocusEvent>
#include <QFont>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QHash>
#include <QHBoxLayout>
#include <QIcon>
#include <QImageReader>
#include <QLineEdit>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLinearGradient>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QPropertyAnimation>
#include <QProxyStyle>
#include <QPushButton>
#include <QRadialGradient>
#include <QRegion>
#include <QResizeEvent>
#include <QScrollBar>
#include <QScrollArea>
#include <QSet>
#include <QSignalBlocker>
#include <QSplitter>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOption>
#include <QSvgRenderer>
#include <QTabBar>
#include <QTabWidget>
#include <QTemporaryFile>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVariant>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>
#include <QWidgetAction>

#include <algorithm>
#include <functional>
#include <cctype>
#include <cstring>
#include <cstdlib>

namespace {

#include "commandtab_globals.inl"
#include "commandtab_icon_cache.inl"
#include "commandtab_model.inl"
#include "commandtab_customization_dialog.inl"
#include "commandtab_settings_dialog.inl"
#include "commandtab_widgets.inl"
#include "commandtab_shell_widget.inl"
#include "commandtab_host_class.inl"

}  // namespace

int freecad_commandtab_native_qt_major_version()
{
#if QT_VERSION_MAJOR >= 6
    return 6;
#else
    return 5;
#endif
}

const char* freecad_commandtab_native_platform_id()
{
#if defined(Q_OS_WIN)
    return "windows";
#elif defined(Q_OS_MACOS)
    return "macos";
#elif defined(Q_OS_LINUX)
    return "linux";
#else
    return "unknown";
#endif
}

bool freecad_commandtab_native_is_supported_platform()
{
    return std::strcmp(freecad_commandtab_native_platform_id(), "unknown") != 0;
}

const char* freecad_commandtab_native_last_error()
{
    return g_lastError.constData();
}

const char* freecad_commandtab_native_build_bootstrap_json(
    const char* structurePath,
    const char* metadataCachePath,
    const char* activeWorkbenchId,
    bool includeAllPanels,
    const char* themeConfigJson,
    const char* settingsJson
)
{
    setLastError(QString());

    if (structurePath == nullptr || metadataCachePath == nullptr || activeWorkbenchId == nullptr) {
        setLastError(QStringLiteral("Missing arguments for bootstrap JSON"));
        return nullptr;
    }

    QJsonObject payload;
    payload.insert(QStringLiteral("kind"), QStringLiteral("bootstrap"));
    payload.insert(QStringLiteral("structurePath"), QString::fromUtf8(structurePath));
    payload.insert(QStringLiteral("metadataCachePath"), QString::fromUtf8(metadataCachePath));
    payload.insert(QStringLiteral("activeWorkbenchId"), QString::fromUtf8(activeWorkbenchId));
    payload.insert(QStringLiteral("includeAllPanels"), includeAllPanels);
    if (themeConfigJson != nullptr) {
        const QJsonObject themeObject = parseJsonObjectText(QString::fromUtf8(themeConfigJson));
        if (!themeObject.isEmpty()) {
            payload.insert(QStringLiteral("themeConfig"), themeObject);
        }
    }
    if (settingsJson != nullptr) {
        const QJsonObject settingsObject = parseJsonObjectText(QString::fromUtf8(settingsJson));
        if (!settingsObject.isEmpty()) {
            payload.insert(QStringLiteral("settings"), settingsObject);
        }
    }

    return duplicateUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
}

const char* freecad_commandtab_native_build_workbench_bootstrap_json(
    const char* structurePath,
    const char* metadataCachePath,
    const char* workbenchId,
    const char* themeConfigJson,
    const char* settingsJson
)
{
    setLastError(QString());

    if (structurePath == nullptr || metadataCachePath == nullptr || workbenchId == nullptr) {
        setLastError(QStringLiteral("Missing arguments for workbench bootstrap JSON"));
        return nullptr;
    }

    QJsonObject payload;
    payload.insert(QStringLiteral("kind"), QStringLiteral("workbench_bootstrap"));
    payload.insert(QStringLiteral("structurePath"), QString::fromUtf8(structurePath));
    payload.insert(QStringLiteral("metadataCachePath"), QString::fromUtf8(metadataCachePath));
    payload.insert(QStringLiteral("workbenchId"), QString::fromUtf8(workbenchId));
    if (themeConfigJson != nullptr) {
        const QJsonObject themeObject = parseJsonObjectText(QString::fromUtf8(themeConfigJson));
        if (!themeObject.isEmpty()) {
            payload.insert(QStringLiteral("themeConfig"), themeObject);
        }
    }
    if (settingsJson != nullptr) {
        const QJsonObject settingsObject = parseJsonObjectText(QString::fromUtf8(settingsJson));
        if (!settingsObject.isEmpty()) {
            payload.insert(QStringLiteral("settings"), settingsObject);
        }
    }

    return duplicateUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
}

const char* freecad_commandtab_native_build_model_json(
    const char* structurePath,
    const char* metadataCachePath,
    const char* activeWorkbenchId,
    bool includeAllPanels,
    const char* themeConfigJson,
    const char* settingsJson
)
{
    setLastError(QString());

    if (structurePath == nullptr || metadataCachePath == nullptr || activeWorkbenchId == nullptr) {
        setLastError(QStringLiteral("Missing arguments for native commandtab model JSON"));
        return nullptr;
    }

    QJsonObject bootstrap;
    bootstrap.insert(QStringLiteral("kind"), QStringLiteral("bootstrap"));
    bootstrap.insert(QStringLiteral("structurePath"), QString::fromUtf8(structurePath));
    bootstrap.insert(QStringLiteral("metadataCachePath"), QString::fromUtf8(metadataCachePath));
    bootstrap.insert(QStringLiteral("activeWorkbenchId"), QString::fromUtf8(activeWorkbenchId));
    bootstrap.insert(QStringLiteral("includeAllPanels"), includeAllPanels);
    if (themeConfigJson != nullptr) {
        const QJsonObject themeObject = parseJsonObjectText(QString::fromUtf8(themeConfigJson));
        if (!themeObject.isEmpty()) {
            bootstrap.insert(QStringLiteral("themeConfig"), themeObject);
        }
    }
    if (settingsJson != nullptr) {
        const QJsonObject settingsObject = parseJsonObjectText(QString::fromUtf8(settingsJson));
        if (!settingsObject.isEmpty()) {
            bootstrap.insert(QStringLiteral("settings"), settingsObject);
        }
    }

    CommandTabModel model;
    if (!buildCommandTabModelFromBootstrap(bootstrap, &model)) {
        return nullptr;
    }

    return duplicateUtf8(QJsonDocument(modelToJsonObject(model)).toJson(QJsonDocument::Compact));
}

const char* freecad_commandtab_native_build_workbench_json(
    const char* structurePath,
    const char* metadataCachePath,
    const char* workbenchId,
    const char* settingsJson
)
{
    setLastError(QString());

    if (structurePath == nullptr || metadataCachePath == nullptr || workbenchId == nullptr) {
        setLastError(QStringLiteral("Missing arguments for native commandtab workbench JSON"));
        return nullptr;
    }

    QJsonObject bootstrap;
    bootstrap.insert(QStringLiteral("kind"), QStringLiteral("workbench_bootstrap"));
    bootstrap.insert(QStringLiteral("structurePath"), QString::fromUtf8(structurePath));
    bootstrap.insert(QStringLiteral("metadataCachePath"), QString::fromUtf8(metadataCachePath));
    bootstrap.insert(QStringLiteral("workbenchId"), QString::fromUtf8(workbenchId));
    if (settingsJson != nullptr) {
        const QJsonObject settingsObject = parseJsonObjectText(QString::fromUtf8(settingsJson));
        if (!settingsObject.isEmpty()) {
            bootstrap.insert(QStringLiteral("settings"), settingsObject);
        }
    }

    CommandTabWorkbenchEntry workbench;
    if (!buildWorkbenchFromBootstrap(bootstrap, &workbench)) {
        return nullptr;
    }

    return duplicateUtf8(QJsonDocument(workbenchToJsonObject(workbench)).toJson(QJsonDocument::Compact));
}

void freecad_commandtab_native_free_string(const char* value)
{
    std::free(const_cast<char*>(value));
}

void* freecad_commandtab_native_create(void* mainWindow, const char* modelPath)
{
    setLastError(QString());

    if (mainWindow == nullptr || modelPath == nullptr) {
        setLastError(QStringLiteral("Missing main window or model path"));
        return nullptr;
    }

    auto* host = new CommandTabHost(reinterpret_cast<QMainWindow*>(mainWindow));
    if (!host->attach(QString::fromUtf8(modelPath))) {
        delete host;
        return nullptr;
    }
    return host;
}

void* freecad_commandtab_native_create_from_json(void* mainWindow, const char* modelJson)
{
    setLastError(QString());

    if (mainWindow == nullptr || modelJson == nullptr) {
        setLastError(QStringLiteral("Missing main window or model JSON"));
        return nullptr;
    }

    auto* host = new CommandTabHost(reinterpret_cast<QMainWindow*>(mainWindow));
    if (!host->attachFromJson(QByteArray(modelJson))) {
        delete host;
        return nullptr;
    }
    return host;
}

void freecad_commandtab_native_destroy(void* handle)
{
    delete reinterpret_cast<CommandTabHost*>(handle);
}

bool freecad_commandtab_native_reload(void* handle, const char* modelPath)
{
    setLastError(QString());

    if (handle == nullptr || modelPath == nullptr) {
        setLastError(QStringLiteral("Missing commandtab handle or model path"));
        return false;
    }

    return reinterpret_cast<CommandTabHost*>(handle)->reload(QString::fromUtf8(modelPath));
}

bool freecad_commandtab_native_reload_json(void* handle, const char* modelJson)
{
    setLastError(QString());

    if (handle == nullptr || modelJson == nullptr) {
        setLastError(QStringLiteral("Missing commandtab handle or model JSON"));
        return false;
    }

    return reinterpret_cast<CommandTabHost*>(handle)->reloadFromJson(QByteArray(modelJson));
}

bool freecad_commandtab_native_reload_from_bootstrap(
    void* handle,
    const char* structurePath,
    const char* metadataCachePath,
    const char* activeWorkbenchId,
    bool includeAllPanels,
    const char* themeConfigJson,
    const char* settingsJson
)
{
    setLastError(QString());

    if (
        handle == nullptr
        || structurePath == nullptr
        || metadataCachePath == nullptr
        || activeWorkbenchId == nullptr
    ) {
        setLastError(QStringLiteral("Missing arguments for bootstrap reload"));
        return false;
    }

    return reinterpret_cast<CommandTabHost*>(handle)->reloadFromBootstrap(
        QString::fromUtf8(structurePath),
        QString::fromUtf8(metadataCachePath),
        QString::fromUtf8(activeWorkbenchId),
        includeAllPanels,
        themeConfigJson != nullptr ? QString::fromUtf8(themeConfigJson) : QString(),
        settingsJson != nullptr ? QString::fromUtf8(settingsJson) : QString()
    );
}

bool freecad_commandtab_native_open_customization_dialog(
    void* handle,
    const char* workbenchId,
    const char* panelId
)
{
    setLastError(QString());

    if (handle == nullptr) {
        setLastError(QStringLiteral("Native commandtab handle is null"));
        return false;
    }

    return reinterpret_cast<CommandTabHost*>(handle)->openCustomizationDialog(
        workbenchId != nullptr ? QString::fromUtf8(workbenchId) : QString(),
        panelId != nullptr ? QString::fromUtf8(panelId) : QString()
    );
}

bool freecad_commandtab_native_open_settings_dialog(void* handle, const char* settingsJson)
{
    setLastError(QString());

    if (handle == nullptr) {
        setLastError(QStringLiteral("Native commandtab handle is null"));
        return false;
    }
    if (settingsJson == nullptr) {
        setLastError(QStringLiteral("Native commandtab settings JSON is null"));
        return false;
    }

    return reinterpret_cast<CommandTabHost*>(handle)->openSettingsDialog(QByteArray(settingsJson));
}

bool freecad_commandtab_native_apply_settings_json(void* handle, const char* settingsJson)
{
    setLastError(QString());

    if (handle == nullptr) {
        setLastError(QStringLiteral("Native commandtab handle is null"));
        return false;
    }
    if (settingsJson == nullptr) {
        setLastError(QStringLiteral("Native commandtab settings JSON is null"));
        return false;
    }

    return reinterpret_cast<CommandTabHost*>(handle)->applySettings(QByteArray(settingsJson));
}

bool freecad_commandtab_native_export_qt_action_cache(
    void* mainWindow,
    const char* outputPath,
    const char* iconDirectoryPath
)
{
    setLastError(QString());

    if (mainWindow == nullptr || outputPath == nullptr || iconDirectoryPath == nullptr) {
        setLastError(QStringLiteral("Missing main window, output path or icon directory path"));
        return false;
    }

    return exportQtActionCache(
        reinterpret_cast<QMainWindow*>(mainWindow),
        QString::fromUtf8(outputPath),
        QString::fromUtf8(iconDirectoryPath)
    );
}

const char* freecad_commandtab_native_export_qt_action_cache_to_memory(
    void* mainWindow,
    const char* iconDirectoryPath
)
{
    setLastError(QString());

    if (mainWindow == nullptr || iconDirectoryPath == nullptr) {
        setLastError(QStringLiteral("Missing main window or icon directory path"));
        return nullptr;
    }

    QTemporaryFile temporaryFile;
    temporaryFile.setAutoRemove(true);
    if (!temporaryFile.open()) {
        setLastError(QStringLiteral("Unable to create temporary Qt action cache file"));
        return nullptr;
    }
    const QString temporaryPath = temporaryFile.fileName();
    temporaryFile.close();

    if (
        !exportQtActionCache(
            reinterpret_cast<QMainWindow*>(mainWindow),
            temporaryPath,
            QString::fromUtf8(iconDirectoryPath)
        )
    ) {
        return nullptr;
    }

    QFile file(temporaryPath);
    if (!file.open(QIODevice::ReadOnly)) {
        setLastError(QStringLiteral("Unable to read temporary Qt action cache"));
        return nullptr;
    }
    const QByteArray payload = file.readAll();
    file.close();
    return duplicateUtf8(payload);
}

bool freecad_commandtab_native_export_structure_metadata_cache(
    void* mainWindow,
    const char* structurePath,
    const char* outputPath,
    const char* iconDirectoryPath,
    const char* workbenchIdsJson,
    bool includeQuickAccess,
    bool useThemeOverrides
)
{
    setLastError(QString());

    if (
        mainWindow == nullptr
        || structurePath == nullptr
        || outputPath == nullptr
        || iconDirectoryPath == nullptr
        || workbenchIdsJson == nullptr
    ) {
        setLastError(QStringLiteral("Missing arguments for structure metadata cache export"));
        return false;
    }

    return exportStructureMetadataCache(
        reinterpret_cast<QMainWindow*>(mainWindow),
        QString::fromUtf8(structurePath),
        QString::fromUtf8(outputPath),
        QString::fromUtf8(iconDirectoryPath),
        QByteArray(workbenchIdsJson),
        includeQuickAccess,
        useThemeOverrides
    );
}

bool freecad_commandtab_native_build_persistent_metadata_cache(
    void* mainWindow,
    const char* structurePath,
    const char* outputPath,
    const char* iconDirectoryPath,
    bool includeQuickAccess,
    bool useThemeOverrides,
    const char* cacheVersion,
    const char* structureKeyJson,
    const char* themeSignatureJson
)
{
    setLastError(QString());

    if (
        mainWindow == nullptr
        || structurePath == nullptr
        || outputPath == nullptr
        || iconDirectoryPath == nullptr
        || cacheVersion == nullptr
        || structureKeyJson == nullptr
        || themeSignatureJson == nullptr
    ) {
        setLastError(QStringLiteral("Missing arguments for persistent metadata cache"));
        return false;
    }

    QTemporaryFile temporaryFile;
    temporaryFile.setAutoRemove(true);
    if (!temporaryFile.open()) {
        setLastError(QStringLiteral("Unable to create temporary metadata cache file"));
        return false;
    }
    const QString temporaryPath = temporaryFile.fileName();
    temporaryFile.close();

    if (
        !exportStructureMetadataCache(
            reinterpret_cast<QMainWindow*>(mainWindow),
            QString::fromUtf8(structurePath),
            temporaryPath,
            QString::fromUtf8(iconDirectoryPath),
            QByteArray("[]"),
            includeQuickAccess,
            useThemeOverrides
        )
    ) {
        return false;
    }

    QJsonObject metadataRoot;
    if (!loadJsonObjectFromFile(temporaryPath, &metadataRoot, QStringLiteral("temporary structure metadata cache"))) {
        return false;
    }

    QJsonParseError structureKeyError;
    const auto structureKeyDocument = QJsonDocument::fromJson(QByteArray(structureKeyJson), &structureKeyError);
    if (structureKeyError.error != QJsonParseError::NoError || !structureKeyDocument.isArray()) {
        setLastError(QStringLiteral("Invalid structure key JSON for persistent metadata cache"));
        return false;
    }

    QJsonParseError themeSignatureError;
    const auto themeSignatureDocument = QJsonDocument::fromJson(QByteArray(themeSignatureJson), &themeSignatureError);
    if (themeSignatureError.error != QJsonParseError::NoError || !themeSignatureDocument.isArray()) {
        setLastError(QStringLiteral("Invalid theme signature JSON for persistent metadata cache"));
        return false;
    }

    QJsonObject persistentRoot;
    persistentRoot.insert(QStringLiteral("cacheVersion"), QString::fromUtf8(cacheVersion));
    persistentRoot.insert(QStringLiteral("structureKey"), structureKeyDocument.array());
    persistentRoot.insert(QStringLiteral("themeSignature"), themeSignatureDocument.array());
    persistentRoot.insert(QStringLiteral("workbenchTitles"), metadataRoot.value(QStringLiteral("workbenchTitles")).toObject());
    persistentRoot.insert(QStringLiteral("workbenchIcons"), metadataRoot.value(QStringLiteral("workbenchIcons")).toObject());
    persistentRoot.insert(QStringLiteral("panelTitles"), metadataRoot.value(QStringLiteral("panelTitles")).toObject());
    persistentRoot.insert(QStringLiteral("builtWorkbenches"), metadataRoot.value(QStringLiteral("builtWorkbenches")).toArray());
    persistentRoot.insert(QStringLiteral("quickAccessIncluded"), metadataRoot.value(QStringLiteral("quickAccessIncluded")).toBool(includeQuickAccess));
    persistentRoot.insert(QStringLiteral("commands"), metadataRoot.value(QStringLiteral("commands")).toObject());

    return writeJsonObjectToFile(
        QString::fromUtf8(outputPath),
        persistentRoot,
        QStringLiteral("persistent native commandtab metadata cache")
    );
}

bool freecad_commandtab_native_apply_customization_to_structure(
    const char* customizationJson,
    const char* structurePath
)
{
    setLastError(QString());

    if (customizationJson == nullptr || structurePath == nullptr) {
        setLastError(QStringLiteral("Missing customization JSON or structure path"));
        return false;
    }

    return applyCustomizationToStructure(
        QByteArray(customizationJson),
        QString::fromUtf8(structurePath)
    );
}

bool freecad_commandtab_native_export_structure_copy(
    const char* sourcePath,
    const char* targetPath
)
{
    setLastError(QString());

    if (sourcePath == nullptr || targetPath == nullptr) {
        setLastError(QStringLiteral("Missing source or target path for structure export"));
        return false;
    }

    return copyFileReplacing(
        QString::fromUtf8(sourcePath),
        QString::fromUtf8(targetPath),
        QStringLiteral("structure export")
    );
}

bool freecad_commandtab_native_import_structure_copy(
    const char* sourcePath,
    const char* targetPath
)
{
    setLastError(QString());

    if (sourcePath == nullptr || targetPath == nullptr) {
        setLastError(QStringLiteral("Missing source or target path for structure import"));
        return false;
    }

    return copyFileReplacing(
        QString::fromUtf8(sourcePath),
        QString::fromUtf8(targetPath),
        QStringLiteral("structure import")
    );
}

bool freecad_commandtab_native_reset_structure_copy(
    const char* defaultPath,
    const char* targetPath
)
{
    setLastError(QString());

    if (defaultPath == nullptr || targetPath == nullptr) {
        setLastError(QStringLiteral("Missing default or target path for structure reset"));
        return false;
    }

    return copyFileReplacing(
        QString::fromUtf8(defaultPath),
        QString::fromUtf8(targetPath),
        QStringLiteral("structure reset")
    );
}

bool freecad_commandtab_native_restore_structure_from_backup(
    const char* backupDirectory,
    const char* targetPath
)
{
    setLastError(QString());

    if (backupDirectory == nullptr || targetPath == nullptr) {
        setLastError(QStringLiteral("Missing backup directory or target path for structure restore"));
        return false;
    }

    const QDir backupDir(QString::fromUtf8(backupDirectory));
    if (!backupDir.exists()) {
        setLastError(QStringLiteral("Backup directory not found: %1").arg(backupDir.path()));
        return false;
    }

    QFileInfoList candidates = backupDir.entryInfoList(
        QStringList() << QStringLiteral("CommandTabStructure*.json"),
        QDir::Files | QDir::Readable,
        QDir::Time
    );
    if (candidates.isEmpty()) {
        setLastError(QStringLiteral("No commandtab structure backup found in: %1").arg(backupDir.path()));
        return false;
    }

    return copyFileReplacing(
        candidates.first().absoluteFilePath(),
        QString::fromUtf8(targetPath),
        QStringLiteral("structure restore")
    );
}

bool freecad_commandtab_native_set_active_workbench(void* handle, const char* workbenchId)
{
    setLastError(QString());

    if (handle == nullptr || workbenchId == nullptr) {
        setLastError(QStringLiteral("Missing commandtab handle or workbench id"));
        return false;
    }

    return reinterpret_cast<CommandTabHost*>(handle)->setActiveWorkbench(QString::fromUtf8(workbenchId));
}

bool freecad_commandtab_native_upsert_workbench_json(void* handle, const char* workbenchJson, bool activate)
{
    setLastError(QString());

    if (handle == nullptr || workbenchJson == nullptr) {
        setLastError(QStringLiteral("Missing commandtab handle or workbench JSON"));
        return false;
    }

    return reinterpret_cast<CommandTabHost*>(handle)->upsertWorkbenchFromJson(
        QByteArray(workbenchJson),
        activate
    );
}

bool freecad_commandtab_native_upsert_workbench_from_bootstrap(
    void* handle,
    const char* structurePath,
    const char* metadataCachePath,
    const char* workbenchId,
    const char* themeConfigJson,
    const char* settingsJson,
    bool activate
)
{
    setLastError(QString());

    if (
        handle == nullptr
        || structurePath == nullptr
        || metadataCachePath == nullptr
        || workbenchId == nullptr
    ) {
        setLastError(QStringLiteral("Missing arguments for workbench bootstrap upsert"));
        return false;
    }

    return reinterpret_cast<CommandTabHost*>(handle)->upsertWorkbenchFromBootstrap(
        QString::fromUtf8(structurePath),
        QString::fromUtf8(metadataCachePath),
        QString::fromUtf8(workbenchId),
        themeConfigJson != nullptr ? QString::fromUtf8(themeConfigJson) : QString(),
        settingsJson != nullptr ? QString::fromUtf8(settingsJson) : QString(),
        activate
    );
}

bool freecad_commandtab_native_enable_ui_hide_controller(void* handle, bool enabled)
{
    setLastError(QString());

    if (handle == nullptr) {
        setLastError(QStringLiteral("Missing commandtab handle for UI hide controller"));
        return false;
    }

    reinterpret_cast<CommandTabHost*>(handle)->setUiHideControllerEnabled(enabled);
    return true;
}

void freecad_commandtab_native_free_buffer(const char* value)
{
    freecad_commandtab_native_free_string(value);
}

void freecad_commandtab_native_set_command_callback(
    void* handle,
    FreeCADCommandTabNativeCommandCallback callback,
    void* userData
)
{
    if (handle == nullptr) {
        return;
    }
    reinterpret_cast<CommandTabHost*>(handle)->setCommandCallback(callback, userData);
}
