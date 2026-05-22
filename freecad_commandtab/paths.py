import tempfile
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parent.parent
ROOT_DIR_STR = str(ROOT_DIR)
SCRIPTS_DIR = ROOT_DIR / "Scripts"
SCRIPTS_DIR_STR = str(SCRIPTS_DIR)
PACKAGES_DIR = ROOT_DIR / "Resources" / "packages"
PACKAGES_DIR_STR = str(PACKAGES_DIR)
APP_DIR_NAME = "FreeCAD-CommandTab"


def addon_path(*parts: str) -> str:
    return str(ROOT_DIR.joinpath(*parts))


def _freecad_user_app_data_dir() -> Path:
    try:
        import FreeCAD as App

        value = str(App.getUserAppDataDir() or "").strip()
        if value:
            return Path(value)
    except Exception:
        pass
    return Path(tempfile.gettempdir())


def user_state_path(*parts: str) -> str:
    root = _freecad_user_app_data_dir() / APP_DIR_NAME
    return str(root.joinpath(*parts))


def user_cache_path(*parts: str) -> str:
    root = _freecad_user_app_data_dir() / "Cache" / APP_DIR_NAME
    return str(root.joinpath(*parts))
