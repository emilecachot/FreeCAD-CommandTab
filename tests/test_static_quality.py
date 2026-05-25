from __future__ import annotations

import ast
import json
from pathlib import Path

from freecad_commandtab import paths
from tools import package_addon

ROOT = Path(__file__).resolve().parents[1]

FORBIDDEN_NATIVE_RELEASE_PATTERNS = {
    "*.dll.a",
    "*.exp",
    "*.ilk",
    "*.lib",
    "*.pdb",
    "libfreecad_commandtab_native_backend.dll",
    "libgcc_s_seh-1.dll",
    "libstdc++-6.dll",
    "libwinpthread-1.dll",
}


def _function_node(module_path: Path, name: str) -> ast.FunctionDef:
    tree = ast.parse(module_path.read_text(encoding="utf-8"))
    for node in tree.body:
        if isinstance(node, ast.FunctionDef) and node.name == name:
            return node
    raise AssertionError(f"Function not found: {name}")


def test_native_theme_config_has_unique_literal_keys() -> None:
    function = _function_node(ROOT / "freecad_commandtab" / "native" / "bridge.py", "_native_theme_config")
    returns = [node for node in ast.walk(function) if isinstance(node, ast.Return)]
    assert returns

    for return_node in returns:
        value = return_node.value
        if not isinstance(value, ast.Dict):
            continue
        literal_keys = [
            key.value
            for key in value.keys
            if isinstance(key, ast.Constant) and isinstance(key.value, str)
        ]
        assert len(literal_keys) == len(set(literal_keys))


def test_runtime_paths_are_user_scoped() -> None:
    assert "FreeCAD-CommandTab" in paths.user_state_path("CommandTabStructure.json")
    assert "FreeCAD-CommandTab" in paths.user_cache_path("CommandTabNativeMetadata.json")

    bridge_text = (ROOT / "freecad_commandtab" / "native" / "bridge.py").read_text(
        encoding="utf-8"
    )
    bootstrap_text = (ROOT / "freecad_commandtab" / "native" / "bootstrap.py").read_text(
        encoding="utf-8"
    )

    assert 'paths.addon_path("cache", "icons")' not in bridge_text
    assert 'paths.addon_path("CommandTabNativeMetadata.json")' not in bridge_text
    assert 'paths.addon_path("FREECAD_COMMANDTAB_NATIVE_STATUS.json")' not in bootstrap_text


def test_native_controller_exposes_close_lifecycle() -> None:
    controller = _function_node(
        ROOT / "freecad_commandtab" / "native" / "bridge.py",
        "activate_native_commandtab",
    )
    assert controller.name == "activate_native_commandtab"

    tree = ast.parse((ROOT / "freecad_commandtab" / "native" / "bridge.py").read_text(encoding="utf-8"))
    classes = [node for node in tree.body if isinstance(node, ast.ClassDef)]
    native_controller = next(node for node in classes if node.name == "NativeCommandTabController")
    method_names = {node.name for node in native_controller.body if isinstance(node, ast.FunctionDef)}

    assert "close" in method_names
    assert "__del__" in method_names

    module_text = (ROOT / "freecad_commandtab" / "native" / "bridge.py").read_text(
        encoding="utf-8"
    )
    assert "def shutdown_native_commandtab()" in module_text
    assert "freecad_commandtab_native_destroy" in module_text


def test_package_script_excludes_generated_state() -> None:
    assert package_addon.should_skip(ROOT / "build")
    assert package_addon.should_skip(ROOT / "dist")
    assert package_addon.should_skip(ROOT / "_native_artifacts")
    assert package_addon.should_skip(ROOT / ".venv")
    assert package_addon.should_skip(ROOT / "freecad_commandtab" / "native" / "bin" / "windows" / "qt6" / "freecad_commandtab_native_backend.pdb")
    assert package_addon.should_skip(ROOT / "freecad_commandtab" / "native" / "bin" / "windows" / "qt6" / "freecad_commandtab_native_backend.ilk")
    assert package_addon.should_skip(ROOT / "freecad_commandtab" / "native" / "bin" / "windows" / "qt6" / "freecad_commandtab_native_backend.lib")
    assert package_addon.should_skip(ROOT / "freecad_commandtab" / "native" / "bin" / "windows" / "qt6" / "libfreecad_commandtab_native_backend.dll.a")
    assert package_addon.should_skip(ROOT / "freecad_commandtab" / "native" / "bin" / "windows" / "qt6" / "libfreecad_commandtab_native_backend.dll")
    assert package_addon.should_skip(ROOT / "freecad_commandtab" / "native" / "bin" / "windows" / "qt6" / "libgcc_s_seh-1.dll")
    assert package_addon.should_skip(ROOT / "freecad_commandtab" / "native" / "bin" / "windows" / "qt6" / "libstdc++-6.dll")
    assert package_addon.should_skip(ROOT / "freecad_commandtab" / "native" / "bin" / "windows" / "qt6" / "libwinpthread-1.dll")
    assert package_addon.should_skip(ROOT / "CommandTabNativeMetadata.json")
    assert package_addon.should_skip(ROOT / "FREECAD_COMMANDTAB_NATIVE_STATUS.json")
    assert package_addon.should_skip(ROOT / "CommandTabDataFile2.dat")
    assert package_addon.should_skip(ROOT / "CommandTabStructure.json")
    assert package_addon.should_skip(ROOT / "CommandTabStructure_default.json")
    assert not package_addon.should_skip(
        ROOT / "freecad_commandtab" / "native" / "bin" / "linux" / "qt6" / "libfreecad_commandtab_native_backend.so"
    )
    assert not package_addon.should_skip(
        ROOT / "freecad_commandtab" / "native" / "bin" / "macos" / "qt6" / "libfreecad_commandtab_native_backend.dylib"
    )
    assert not package_addon.should_skip(
        ROOT / "freecad_commandtab" / "native" / "bin" / "windows" / "qt6" / "freecad_commandtab_native_backend.dll"
    )
    assert package_addon.should_skip(
        ROOT / "freecad_commandtab" / "core" / "CommandTabStructure.json"
    )


def test_source_tree_does_not_ship_native_build_byproducts() -> None:
    native_bin = ROOT / "freecad_commandtab" / "native" / "bin"
    offenders = [
        path.relative_to(ROOT).as_posix()
        for path in native_bin.rglob("*")
        if path.is_file()
        and any(path.match(pattern) for pattern in FORBIDDEN_NATIVE_RELEASE_PATTERNS)
    ]
    assert offenders == []


def test_windows_native_loader_registers_dll_directory() -> None:
    bridge_text = (ROOT / "freecad_commandtab" / "native" / "bridge.py").read_text(
        encoding="utf-8"
    )
    assert '"add_dll_directory"' in bridge_text
    assert "_prepare_windows_dll_search_path(candidate)" in bridge_text
    assert "_WINDOWS_DLL_SEARCH_PATH_HANDLES.append(handle)" in bridge_text


def test_native_dropdown_commands_use_split_button_behavior() -> None:
    bridge_text = (ROOT / "freecad_commandtab" / "native" / "bridge.py").read_text(
        encoding="utf-8"
    )
    widget_text = (
        ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_widgets.inl"
    ).read_text(encoding="utf-8")
    shell_text = (
        ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_shell_widget.inl"
    ).read_text(encoding="utf-8")

    assert "include_plain_command_actions = len(actions) > 1" in bridge_text
    assert "from PySide.QtWidgets import QToolBar, QToolButton, QWidget" in bridge_text
    assert "toolbar.findChildren(QToolButton)" in bridge_text
    assert 'os.environ.get("FREECAD_COMMANDTAB_CPP_BOOTSTRAP_PIPELINE", "0")' in bridge_text
    assert "def _enrich_native_payload_menu_commands" in bridge_text
    assert "_enrich_native_payload_menu_commands(parsed_payload)" in bridge_text
    assert "def _persistent_variant_menu_cache_path() -> Path:" in bridge_text
    assert "def _cached_variant_menu_entries(command_name: str) -> list[dict]:" in bridge_text
    assert "_write_variant_menu_cache_entry(command_name, entries)" in bridge_text
    assert "static_variant_menus.json" in bridge_text
    assert "def _load_static_command_variant_menu_specs() -> dict[str, list[object]]:" in bridge_text
    assert '"Sketcher_CompCreateRectangles"' in bridge_text
    assert '"Sketcher_CreateRectangle_Center"' in bridge_text
    assert "_cached_or_static_variant_menu_entries(command_name, command_data)" in bridge_text
    assert "dropdownHotZoneRect().contains(event->pos())" in widget_text
    assert 'setProperty("commandtabHasMenuCommands", !m_menuCommands.isEmpty())' in widget_text
    assert 'setProperty("commandtabMenuCommandCount", m_menuCommands.size())' in widget_text
    assert "return scaledPx(15)" in widget_text
    assert "shouldShowMenu" in widget_text
    assert "QToolButton::MenuButtonPopup" in shell_text
    assert "commandtabHasMenuCommands" in shell_text
    assert "commandtabMenuCommandCount" in shell_text
    assert "def _load_static_command_variant_menu_specs() -> dict[str, list[object]]:" in bridge_text


def test_native_command_enable_state_refreshes_globally() -> None:
    shell_text = (
        ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_shell_widget.inl"
    ).read_text(encoding="utf-8")

    assert "refreshCommandStatesFromActions()" in shell_text
    assert "scheduleCommandStateRefresh(20, 6)" in shell_text
    assert "scheduleCommandStateRefresh(80, 5)" in shell_text
    assert "startCommandStatePolling()" in shell_text
    assert "commandtabEnabledBindingCommandId" in shell_text
    assert "commandtabCheckedBindingCommandId" in shell_text
    assert "m_enabledWidgetsByCommandId" in shell_text
    assert "m_checkedButtonsByCommandId" in shell_text
    assert "m_commandStatePollTimer->setInterval(2000)" in shell_text
    assert "refreshCommandStatesAfterWorkbenchChange()" in shell_text
    assert "scheduleCommandStatesAfterWorkbenchChange" in shell_text
    assert "scheduleWorkbenchActivation(index)" in shell_text
    assert "ensureWorkbenchPage(index, 2, true)" in shell_text
    assert "scheduleCommandStateRefresh(0, 3)" in shell_text


def test_build_script_uses_release_config_by_default() -> None:
    build_script_text = (ROOT / "tools" / "build_and_package_local.py").read_text(
        encoding="utf-8"
    )
    assert 'DEFAULT_BUILD_CONFIG = "Release"' in build_script_text
    assert '"--config"' in build_script_text
    assert "args.build_config" in build_script_text
    assert "copy_windows_mingw_runtime_dependencies(output_dir)" in build_script_text


def test_package_script_includes_native_mode_marker(tmp_path: Path) -> None:
    output = tmp_path / "FreeCAD-CommandTab"
    package_addon.copy_tree(output)
    assert (output / package_addon.NATIVE_MODE_MARKER).is_file()


def test_critical_runtime_paths_exist() -> None:
    assert Path(paths.ROOT_DIR_STR).is_dir()
    assert Path(paths.SCRIPTS_DIR_STR).is_dir()
    assert Path(paths.PACKAGES_DIR_STR).is_dir()
    assert Path(paths.addon_path("Resources", "icons")).is_dir()
    assert Path(paths.addon_path("Resources", "stylesheets")).is_dir()
    assert Path(paths.addon_path("CreateStructure.txt")).is_file()


def test_viewport_style_settings_contract_present() -> None:
    bridge_text = (ROOT / "freecad_commandtab" / "native" / "bridge.py").read_text(
        encoding="utf-8"
    )
    model_text = (ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_model.inl").read_text(
        encoding="utf-8"
    )
    dialog_text = (ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_settings_dialog.inl").read_text(
        encoding="utf-8"
    )

    assert "ViewportBackgroundStyle" in bridge_text
    assert "ViewportBgMidColor" in bridge_text
    assert "ViewportBgAccentColor" in bridge_text
    assert "viewportBackgroundStyle" in model_text
    assert "viewportBgMidColor" in model_text
    assert "viewportBgAccentColor" in model_text
    assert "Tri-color gradient" in dialog_text
    assert "Pattern (4 corners)" in dialog_text


def test_viewport_settings_read_from_runtime_settings_group() -> None:
    bridge_text = (ROOT / "freecad_commandtab" / "native" / "bridge.py").read_text(
        encoding="utf-8"
    )
    assert "FreeCAD-CommandTab/Settings" in bridge_text
    assert "_runtime_bool_setting(\"ViewportColorsEnabled\"" in bridge_text
    assert "_runtime_string_setting(\"ViewportBackgroundStyle\"" in bridge_text
    assert "_runtime_color_setting(\"ViewportBgTopColor\"" in bridge_text


def test_native_metadata_cache_tracks_freecad_environment_signature() -> None:
    bridge_text = (ROOT / "freecad_commandtab" / "native" / "bridge.py").read_text(
        encoding="utf-8"
    )
    assert "def _native_metadata_environment_signature()" in bridge_text
    assert "App.Version()" in bridge_text
    assert "_available_workbenches().keys()" in bridge_text
    assert '"environmentSignature"' in bridge_text
    assert "def _cached_command_metadata_is_complete(command_payload: dict) -> bool:" in bridge_text
    assert 'if "iconPath" not in payload:' in bridge_text
    assert "including absent iconPath" in bridge_text
    assert "icon_path == \"\" or _is_stable_icon_reference(icon_path)" in bridge_text
    assert "command_metadata_workbenches = (" in bridge_text
    assert "target_workbenches if len(missing_command_metadata) > 0 else workbenches_to_build" in bridge_text
    assert "structure_digest = hashlib.sha1(structure_bytes).hexdigest()" in bridge_text
    assert "cache_key = (str(structure_path), structure_digest, len(structure_bytes))" in bridge_text


def test_ribbon_surface_settings_contract_present() -> None:
    bridge_text = (ROOT / "freecad_commandtab" / "native" / "bridge.py").read_text(
        encoding="utf-8"
    )
    model_text = (ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_model.inl").read_text(
        encoding="utf-8"
    )
    dialog_text = (ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_settings_dialog.inl").read_text(
        encoding="utf-8"
    )
    shell_text = (ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_shell_widget.inl").read_text(
        encoding="utf-8"
    )
    widget_text = (ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_widgets.inl").read_text(
        encoding="utf-8"
    )

    assert "RibbonSurfaceStyle" in bridge_text
    assert "RibbonSurfaceTransparent" in bridge_text
    assert "NativeButtonBordersVisible" in bridge_text
    assert "ribbonSurfaceStyle" in model_text
    assert "ribbonSurfaceTransparent" in model_text
    assert "buttonBordersVisible" in model_text
    assert "Ribbon surface" in dialog_text
    assert "Use transparent ribbon surfaces" in dialog_text
    assert "Show button outlines" in dialog_text
    assert "surfaceStyleChanged" in shell_text
    assert "buttonBordersChanged" in shell_text
    assert "m_surfaceStyle" in widget_text
    assert "m_buttonBordersVisible" in widget_text


def test_ondsel_defaults_can_be_disabled() -> None:
    parameters_text = (ROOT / "Parameters_CommandTab.py").read_text(encoding="utf-8")
    bootstrap_text = (ROOT / "freecad_commandtab" / "native" / "bootstrap.py").read_text(
        encoding="utf-8"
    )
    bridge_text = (ROOT / "freecad_commandtab" / "native" / "bridge.py").read_text(
        encoding="utf-8"
    )
    model_text = (ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_model.inl").read_text(
        encoding="utf-8"
    )
    dialog_text = (ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_settings_dialog.inl").read_text(
        encoding="utf-8"
    )

    assert "ApplyOndselDefaults" in parameters_text
    assert "APPLY_ONDSEL_DEFAULTS" in bootstrap_text
    assert "_apply_ondsel_defaults_once()" in bootstrap_text
    assert "ONDSEL_DEFAULTS_BACKUP_KEY" in bootstrap_text
    assert "_restore_ondsel_defaults_if_disabled()" in bootstrap_text
    assert "backup_key=ONDSEL_DEFAULTS_BACKUP_KEY" in bootstrap_text
    assert "current_value != entry.get(\"target\")" in bootstrap_text
    assert "applyOndselDefaults" in bridge_text
    assert "applyOndselDefaults" in model_text
    assert "Apply Ondsel defaults at startup" in dialog_text


def test_workspace_tree_overlay_is_explicit_not_tied_to_ribbon_transparency() -> None:
    bootstrap_text = (ROOT / "freecad_commandtab" / "native" / "bootstrap.py").read_text(
        encoding="utf-8"
    )
    bridge_text = (ROOT / "freecad_commandtab" / "native" / "bridge.py").read_text(
        encoding="utf-8"
    )

    assert "_workspace_tree_overlay_requested" in bootstrap_text
    assert "Ribbon material transparency is visual only" in bootstrap_text
    assert "USE_FC_OVERLAY" in bootstrap_text
    assert "_configure_transparent_workspace_tree_overlay()" in bootstrap_text
    assert '"Tree view", "Property view"' in bootstrap_text
    assert '_set_bool_if_changed(overlay_left, "Transparent", True)' in bootstrap_text
    assert '_set_bool_if_changed(dock_windows, "Std_ComboView", False)' in bootstrap_text
    assert "workbench_name in {\"NoneWorkbench\"}" in bridge_text
    assert 'FREECAD_COMMANDTAB_STARTUP_PRELOAD_ALL_PANELS", "0"' in bridge_text
    assert "fallback makes startup visibly slower" in bridge_text
    assert "OVERLAY_DISABLED" in bootstrap_text


def test_part_design_sketch_compound_label_is_corrected() -> None:
    bridge_text = (ROOT / "freecad_commandtab" / "native" / "bridge.py").read_text(
        encoding="utf-8"
    )
    model_text = (ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_model.inl").read_text(
        encoding="utf-8"
    )

    assert '"PartDesign_CompSketches": ("CmdPartDesignNewSketch", "New Sketch")' in bridge_text
    assert 'commandId == QStringLiteral("PartDesign_CompSketches")' in model_text
    assert 'QCoreApplication::translate("CmdPartDesignNewSketch", "New Sketch")' in model_text


def test_native_icon_cache_preloads_static_variant_children() -> None:
    bridge_text = (ROOT / "freecad_commandtab" / "native" / "bridge.py").read_text(
        encoding="utf-8"
    )
    shell_text = (ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_shell_widget.inl").read_text(
        encoding="utf-8"
    )

    assert "def _static_variant_child_command_ids(command_name: str) -> list[str]:" in bridge_text
    assert "for child_command_name in _static_variant_child_command_ids(command_name)" in bridge_text
    assert "rebuildIconPreloadQueue()" in shell_text
    assert "scheduleIconPreload(0)" in shell_text
    assert "loadCommandEntryIcon(m_iconPreloadQueue.at(m_iconPreloadCursor))" in shell_text


def test_workbench_tab_icons_are_cached_and_stable() -> None:
    bridge_text = (ROOT / "freecad_commandtab" / "native" / "bridge.py").read_text(
        encoding="utf-8"
    )
    shell_text = (ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_shell_widget.inl").read_text(
        encoding="utf-8"
    )

    assert "_workbench_icon_export_path(workbench_name, persistent=True)" in bridge_text
    assert "if persistent_path.exists()" in bridge_text
    assert "m_workbenchTabIconCache" in shell_text
    assert "if (!m_tabBar->tabIcon(tabIndex).isNull())" in shell_text


def test_native_background_preload_yields_to_ribbon_interaction() -> None:
    shell_text = (
        ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_shell_widget.inl"
    ).read_text(encoding="utf-8")

    assert "commandtabInteractionFilterInstalled" in shell_text
    assert "isRibbonEventTarget(watched)" in shell_text
    assert "shouldYieldBackgroundRibbonWork" in shell_text
    assert "ensureWorkbenchPage(index, 100000, false)" not in shell_text
    assert "ensureWorkbenchPage(index, 2, true)" in shell_text
    assert "const int budget = 8" in shell_text
    assert "const int budget = currentPage && !shouldYieldBackgroundRibbonWork(80) ? 5 : 2" in shell_text
    assert "QTimer::singleShot(std::clamp(delayMs, 30, 1200)" in shell_text


def test_command_button_hover_and_labels_are_latency_aware() -> None:
    widget_text = (
        ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_widgets.inl"
    ).read_text(encoding="utf-8")

    assert "m_baseFontPointSize" in widget_text
    assert "minimumFitPointSize" in widget_text
    assert "linesFitWidth(candidateLines" in widget_text
    assert "repaint();" in widget_text
    assert "paintCachedIcon" in widget_text
    assert "m_cachedIconPixmap" in widget_text


def test_command_icon_fallback_avoids_native_file_document_icon() -> None:
    icon_text = (
        ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_icon_cache.inl"
    ).read_text(encoding="utf-8")

    assert "genericCommandFallbackGlyph" in icon_text
    assert "QStyle::SP_FileIcon" not in icon_text
    assert "genericCommandFallbackIcon(fallbackLabel)" in icon_text


def test_panel_footer_uses_antialiased_painted_surface() -> None:
    widget_text = (ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_widgets.inl").read_text(
        encoding="utf-8"
    )
    shell_text = (ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_shell_widget.inl").read_text(
        encoding="utf-8"
    )

    assert "class CommandTabPanelFooterWidget final" in widget_text
    assert "painter.setRenderHint(QPainter::Antialiasing, true)" in widget_text
    assert "new CommandTabPanelFooterWidget(&m_theme, m_settingsState, panelWidget)" in shell_text
    assert "setProperty(\"commandtabPanelDisplayTitle\", panelDisplayTitle)" in shell_text


def test_native_runtime_and_bootstrap_payload_cache_are_user_scoped() -> None:
    bridge_text = (ROOT / "freecad_commandtab" / "native" / "bridge.py").read_text(
        encoding="utf-8"
    )
    assert 'Path(paths.user_cache_path("native-runtime"))' in bridge_text
    assert "def _runtime_bootstrap_payload_cache_path()" in bridge_text
    assert '"bootstrap-payload-cache.json"' in bridge_text


def test_loaded_workbench_activation_prefers_set_active_without_upsert() -> None:
    bridge_text = (ROOT / "freecad_commandtab" / "native" / "bridge.py").read_text(
        encoding="utf-8"
    )
    assert "if workbench_name in self._loaded_workbenches:" in bridge_text
    assert "if self._set_active_workbench(workbench_name):" in bridge_text


def test_separator_normalization_uses_shared_helper() -> None:
    bridge_text = (ROOT / "freecad_commandtab" / "native" / "bridge.py").read_text(
        encoding="utf-8"
    )
    settings_text = (
        ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_settings_dialog.inl"
    ).read_text(encoding="utf-8")
    assert "def _is_separator_command_id(command_name: str) -> bool:" in bridge_text
    assert 'normalized.startswith("separator_")' in bridge_text
    assert 'normalized in {"0", "|", "-", "--", "---", "separator"}' in bridge_text
    assert "if _is_separator_command_id(command_name):" in bridge_text
    assert "bool isSeparatorCommand(const QString& commandId)" in settings_text
    assert "normalized.startsWith(QStringLiteral(\"separator_\"))" in settings_text
    assert "if (!id.isEmpty() && !isSeparatorCommand(id)) {" in settings_text


def test_panel_command_identifier_resolution_is_shared() -> None:
    bridge_text = (ROOT / "freecad_commandtab" / "native" / "bridge.py").read_text(
        encoding="utf-8"
    )
    assert "def _resolve_panel_command_identifier(command_name: str, command_map: dict) -> str:" in bridge_text
    assert "if candidate in command_map:" in bridge_text
    assert "text_matches = []" in bridge_text
    assert "command_name = _resolve_panel_command_identifier(" in bridge_text


def test_grid_buttons_use_stable_native_icon() -> None:
    icon_cache_text = (
        ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_icon_cache.inl"
    ).read_text(encoding="utf-8")
    shell_text = (
        ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_shell_widget.inl"
    ).read_text(encoding="utf-8")
    widget_text = (
        ROOT / "freecad_commandtab" / "native" / "cpp" / "src" / "commandtab_widgets.inl"
    ).read_text(encoding="utf-8")

    assert "bool isGridToggleCommandId(const QString& commandId)" in icon_cache_text
    assert "QIcon gridToggleFallbackIcon()" in icon_cache_text
    assert "Sketcher_GridToggle_Deactivated.svg" in icon_cache_text
    assert "isGridToggleCommandId(command.id)" in shell_text
    assert "gridToggleFallbackIcon()" in shell_text
    assert "!isGridToggleCommandId(command.id)" in widget_text


def test_startup_variant_menu_repair_is_enabled() -> None:
    bridge_text = (ROOT / "freecad_commandtab" / "native" / "bridge.py").read_text(
        encoding="utf-8"
    )
    static_variant_menus = json.loads(
        (ROOT / "freecad_commandtab" / "native" / "static_variant_menus.json").read_text(
            encoding="utf-8"
        )
    )
    assert "_NATIVE_BOOTSTRAP_PAYLOAD_CACHE_VERSION = 2" in bridge_text
    assert "_NATIVE_VARIANT_MENU_CACHE_VERSION = 1" in bridge_text
    assert "_NATIVE_METADATA_CACHE_VERSION = 36" in bridge_text
    assert "def _payload_has_command_variant_menus(payload: str) -> bool:" in bridge_text
    assert 'command_type == "command"' in bridge_text
    assert "CommandTabVariantMenus.json" in bridge_text
    assert "return _cached_or_static_variant_menu_entries(command_name, command_data)" in bridge_text
    assert "def _static_variant_menu_entries(command_name: str, command_data: dict) -> list[dict]:" in bridge_text
    assert "def _schedule_variant_menu_repair_if_needed(self, payload: str) -> None:" in bridge_text
    assert "def _run_variant_menu_repair(self) -> None:" in bridge_text
    assert "self._schedule_variant_menu_repair_if_needed(payload)" in bridge_text
    assert "def _clear_runtime_payload_caches() -> None:" in bridge_text
    assert "_clear_runtime_payload_caches()" in bridge_text
    assert "_build_native_model_payload_safe(" in bridge_text
    assert "self._last_bootstrap_state = {}" in bridge_text
    assert "_NATIVE_VARIANT_MENU_REPAIR_MAX_ATTEMPTS" in bridge_text
    assert "self._variant_menu_repair_attempt_count += 1" in bridge_text
    assert static_variant_menus["cacheVersion"] == 1
    assert "Sketcher_CompCreateRectangles" in static_variant_menus["commands"]
    assert "Sketcher_CreateRectangle_Center" in static_variant_menus["commands"]["Sketcher_CompCreateRectangles"]
    assert "TechDraw_CompDimensionTools" in static_variant_menus["commands"]
    assert "PartDesign_CompPrimitiveAdditive" in static_variant_menus["commands"]
