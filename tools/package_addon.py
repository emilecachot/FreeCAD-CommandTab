#!/usr/bin/env python3
from __future__ import annotations

import argparse
import fnmatch
import shutil
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = Path(tempfile.gettempdir()) / "FreeCAD-CommandTab-package"

EXCLUDED_DIRS = {
    ".git",
    ".github",
    ".claude",
    ".pytest_cache",
    ".ruff_cache",
    ".qt",
    ".venv",
    ".venv-qt68",
    "__pycache__",
    "build",
    "dist",
    "tests",
    "tools",
    "venv",
}

EXCLUDED_FILE_PATTERNS = {
    "*.pyc",
    "*.pyo",
    "*.dll.a",
    "*.exp",
    "*.ilk",
    "*.lib",
    "*.pdb",
    "*.bak",
    "*~",
    ".coverage",
}

EXCLUDED_FILE_NAMES = {
    "libfreecad_commandtab_native_backend.dll",
    "libgcc_s_seh-1.dll",
    "libstdc++-6.dll",
    "libwinpthread-1.dll",
}

EXCLUDED_ROOT_FILE_PATTERNS = {
    "FREECAD_COMMANDTAB_NATIVE_STATUS.json",
    "CommandTabNativeMetadata.json",
    "CommandTabStructure.json",
    "CommandTabStructure_default.json",
    "CommandTabDataFile*.dat",
}

EXCLUDED_RELATIVE_PATH_PREFIXES = {
    "freecad_commandtab/core",
    "freecad_commandtab/dialogs",
}

NATIVE_MODE_MARKER = "FREECAD_COMMANDTAB_NATIVE"

def should_skip(path: Path) -> bool:
    relative = path.relative_to(ROOT)
    relative_posix = relative.as_posix()
    if any(
        relative_posix == prefix or relative_posix.startswith(prefix + "/")
        for prefix in EXCLUDED_RELATIVE_PATH_PREFIXES
    ):
        return True
    if any(part in EXCLUDED_DIRS for part in relative.parts):
        return True
    # Exclude local out-of-tree build directories like build-review/build-analyzer.
    if any(part.startswith("build-") for part in relative.parts):
        return True
    # Exclude generated user-state files only when they sit at repository root.
    if len(relative.parts) == 1 and any(
        fnmatch.fnmatch(path.name, pattern)
        for pattern in EXCLUDED_ROOT_FILE_PATTERNS
    ):
        return True
    if any(fnmatch.fnmatch(path.name, pattern) for pattern in EXCLUDED_FILE_PATTERNS):
        return True
    if path.name in EXCLUDED_FILE_NAMES:
        return True
    return False


def copy_tree(output: Path) -> None:
    if output.exists():
        shutil.rmtree(output)
    output.mkdir(parents=True)

    for source in sorted(ROOT.rglob("*")):
        if source == output or output in source.parents:
            continue
        if should_skip(source):
            continue

        target = output / source.relative_to(ROOT)
        if source.is_dir():
            target.mkdir(parents=True, exist_ok=True)
            continue

        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)

    # Create a marker file to enable native-only mode when the addon is installed.
    native_mode_marker = output / NATIVE_MODE_MARKER
    native_mode_marker.parent.mkdir(parents=True, exist_ok=True)
    native_mode_marker.touch(exist_ok=True)


def main() -> int:
    parser = argparse.ArgumentParser(description="Build a clean FreeCAD CommandTab addon folder.")
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help=f"Output directory, default: {DEFAULT_OUTPUT}",
    )
    args = parser.parse_args()

    output = args.output.resolve()
    copy_tree(output)
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
