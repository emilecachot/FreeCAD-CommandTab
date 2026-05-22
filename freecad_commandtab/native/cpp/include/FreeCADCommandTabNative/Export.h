#pragma once

#if defined(_WIN32)
#  if defined(FCRNATIVE_BUILD)
#    define FCRNATIVE_EXPORT __declspec(dllexport)
#  else
#    define FCRNATIVE_EXPORT __declspec(dllimport)
#  endif
#else
#  define FCRNATIVE_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {

typedef void (*FreeCADCommandTabNativeCommandCallback)(const char* command, void* userData);

FCRNATIVE_EXPORT int freecad_commandtab_native_qt_major_version();
FCRNATIVE_EXPORT const char* freecad_commandtab_native_platform_id();
FCRNATIVE_EXPORT bool freecad_commandtab_native_is_supported_platform();
FCRNATIVE_EXPORT const char* freecad_commandtab_native_last_error();
FCRNATIVE_EXPORT const char* freecad_commandtab_native_build_bootstrap_json(
    const char* structurePath,
    const char* metadataCachePath,
    const char* activeWorkbenchId,
    bool includeAllPanels,
    const char* themeConfigJson,
    const char* settingsJson
);
FCRNATIVE_EXPORT const char* freecad_commandtab_native_build_workbench_bootstrap_json(
    const char* structurePath,
    const char* metadataCachePath,
    const char* workbenchId,
    const char* themeConfigJson,
    const char* settingsJson
);
FCRNATIVE_EXPORT const char* freecad_commandtab_native_build_model_json(
    const char* structurePath,
    const char* metadataCachePath,
    const char* activeWorkbenchId,
    bool includeAllPanels,
    const char* themeConfigJson,
    const char* settingsJson
);
FCRNATIVE_EXPORT const char* freecad_commandtab_native_build_workbench_json(
    const char* structurePath,
    const char* metadataCachePath,
    const char* workbenchId,
    const char* settingsJson
);
FCRNATIVE_EXPORT void freecad_commandtab_native_free_string(const char* value);
FCRNATIVE_EXPORT void* freecad_commandtab_native_create(void* mainWindow, const char* modelPath);
FCRNATIVE_EXPORT void* freecad_commandtab_native_create_from_json(void* mainWindow, const char* modelJson);
FCRNATIVE_EXPORT void freecad_commandtab_native_destroy(void* handle);
FCRNATIVE_EXPORT bool freecad_commandtab_native_reload(void* handle, const char* modelPath);
FCRNATIVE_EXPORT bool freecad_commandtab_native_reload_json(void* handle, const char* modelJson);
FCRNATIVE_EXPORT bool freecad_commandtab_native_reload_from_bootstrap(
    void* handle,
    const char* structurePath,
    const char* metadataCachePath,
    const char* activeWorkbenchId,
    bool includeAllPanels,
    const char* themeConfigJson,
    const char* settingsJson
);
FCRNATIVE_EXPORT bool freecad_commandtab_native_open_customization_dialog(
    void* handle,
    const char* workbenchId,
    const char* panelId
);
FCRNATIVE_EXPORT bool freecad_commandtab_native_open_settings_dialog(
    void* handle,
    const char* settingsJson
);
FCRNATIVE_EXPORT bool freecad_commandtab_native_apply_settings_json(
    void* handle,
    const char* settingsJson
);
FCRNATIVE_EXPORT bool freecad_commandtab_native_export_qt_action_cache(
    void* mainWindow,
    const char* outputPath,
    const char* iconDirectoryPath
);
FCRNATIVE_EXPORT const char* freecad_commandtab_native_export_qt_action_cache_to_memory(
    void* mainWindow,
    const char* iconDirectoryPath
);
FCRNATIVE_EXPORT bool freecad_commandtab_native_export_structure_metadata_cache(
    void* mainWindow,
    const char* structurePath,
    const char* outputPath,
    const char* iconDirectoryPath,
    const char* workbenchIdsJson,
    bool includeQuickAccess,
    bool useThemeOverrides
);
FCRNATIVE_EXPORT bool freecad_commandtab_native_build_persistent_metadata_cache(
    void* mainWindow,
    const char* structurePath,
    const char* outputPath,
    const char* iconDirectoryPath,
    bool includeQuickAccess,
    bool useThemeOverrides,
    const char* cacheVersion,
    const char* structureKeyJson,
    const char* themeSignatureJson
);
FCRNATIVE_EXPORT bool freecad_commandtab_native_apply_customization_to_structure(
    const char* customizationJson,
    const char* structurePath
);
FCRNATIVE_EXPORT bool freecad_commandtab_native_export_structure_copy(
    const char* sourcePath,
    const char* targetPath
);
FCRNATIVE_EXPORT bool freecad_commandtab_native_import_structure_copy(
    const char* sourcePath,
    const char* targetPath
);
FCRNATIVE_EXPORT bool freecad_commandtab_native_reset_structure_copy(
    const char* defaultPath,
    const char* targetPath
);
FCRNATIVE_EXPORT bool freecad_commandtab_native_restore_structure_from_backup(
    const char* backupDirectory,
    const char* targetPath
);
FCRNATIVE_EXPORT bool freecad_commandtab_native_set_active_workbench(void* handle, const char* workbenchId);
FCRNATIVE_EXPORT bool freecad_commandtab_native_upsert_workbench_json(
    void* handle,
    const char* workbenchJson,
    bool activate
);
FCRNATIVE_EXPORT bool freecad_commandtab_native_upsert_workbench_from_bootstrap(
    void* handle,
    const char* structurePath,
    const char* metadataCachePath,
    const char* workbenchId,
    const char* themeConfigJson,
    const char* settingsJson,
    bool activate
);
FCRNATIVE_EXPORT bool freecad_commandtab_native_enable_ui_hide_controller(void* handle, bool enabled);
FCRNATIVE_EXPORT void freecad_commandtab_native_free_buffer(const char* value);
FCRNATIVE_EXPORT void freecad_commandtab_native_set_command_callback(
    void* handle,
    FreeCADCommandTabNativeCommandCallback callback,
    void* userData
);

}
