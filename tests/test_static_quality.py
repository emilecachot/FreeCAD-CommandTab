from __future__ import annotations

import ast
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
    assert "dropdownHotZoneRect().contains(event->pos())" in widget_text
    assert 'setProperty("commandtabHasMenuCommands", !m_menuCommands.isEmpty())' in widget_text
    assert 'setProperty("commandtabMenuCommandCount", m_menuCommands.size())' in widget_text
    assert "return scaledPx(15)" in widget_text
    assert "shouldShowMenu" in widget_text
    assert "QToolButton::MenuButtonPopup" in shell_text
    assert "commandtabHasMenuCommands" in shell_text
    assert "commandtabMenuCommandCount" in shell_text


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
