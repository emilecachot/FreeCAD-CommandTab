#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path

EXPECTED_PLATFORMS = ("linux", "windows", "macos")
EXPECTED_QT = ("qt5", "qt6")
PLATFORM_ALIASES = {
    "darwin": "macos",
    "mac": "macos",
    "osx": "macos",
}


def _split_values(values: list[str] | None, *, aliases: dict[str, str] | None = None) -> tuple[str, ...]:
    if not values:
        return ()

    normalized: list[str] = []
    aliases = aliases or {}
    for value in values:
        for part in str(value).split(","):
            item = part.strip().lower()
            if not item:
                continue
            item = aliases.get(item, item)
            if item not in normalized:
                normalized.append(item)
    return tuple(normalized)


def expected_entries(
    platforms: tuple[str, ...] = EXPECTED_PLATFORMS,
    qts: tuple[str, ...] = EXPECTED_QT,
) -> list[str]:
    return [f"{platform}/{qt}" for platform in platforms for qt in qts]


def detected_entries(
    root: Path,
    platforms: tuple[str, ...] = EXPECTED_PLATFORMS,
    qts: tuple[str, ...] = EXPECTED_QT,
) -> set[str]:
    found: set[str] = set()
    for platform in platforms:
        for qt in qts:
            directory = root / platform / qt
            if directory.is_dir() is False:
                continue
            has_library = any(
                path.is_file()
                for path in directory.iterdir()
                if path.suffix.lower() in [".so", ".dll", ".dylib"]
            )
            if has_library:
                found.add(f"{platform}/{qt}")
    return found


def verify_matrix(
    root: Path,
    platforms: tuple[str, ...] = EXPECTED_PLATFORMS,
    qts: tuple[str, ...] = EXPECTED_QT,
) -> dict[str, object]:
    expected = expected_entries(platforms, qts)
    found = detected_entries(root, platforms, qts)
    missing = [entry for entry in expected if entry not in found]
    return {
        "root": str(root),
        "expected": expected,
        "found": sorted(found),
        "missing": missing,
        "complete": len(missing) == 0,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify native backend binary matrix.")
    parser.add_argument(
        "--root",
        type=Path,
        default=Path("freecad_commandtab/native/bin"),
        help="Root directory containing <platform>/qt<major>/ binaries.",
    )
    parser.add_argument(
        "--require-full-matrix",
        action="store_true",
        help="Return non-zero when any expected platform/Qt entry is missing.",
    )
    parser.add_argument(
        "--platform",
        action="append",
        help=(
            "Restrict verification to one or more platforms. "
            "May be repeated or comma-separated. Darwin/mac/osx map to macos."
        ),
    )
    parser.add_argument(
        "--qt",
        action="append",
        help="Restrict verification to one or more Qt entries, e.g. qt6 or 6.",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Print result as JSON.",
    )
    args = parser.parse_args()

    platforms = _split_values(args.platform, aliases=PLATFORM_ALIASES) or EXPECTED_PLATFORMS
    qts = tuple(
        f"qt{qt}" if qt.isdigit() else qt
        for qt in _split_values(args.qt)
    ) or EXPECTED_QT

    unknown_platforms = [platform for platform in platforms if platform not in EXPECTED_PLATFORMS]
    unknown_qts = [qt for qt in qts if qt not in EXPECTED_QT]
    if unknown_platforms:
        parser.error(f"unknown platform(s): {', '.join(unknown_platforms)}")
    if unknown_qts:
        parser.error(f"unknown Qt entry/entries: {', '.join(unknown_qts)}")

    result = verify_matrix(args.root, platforms=platforms, qts=qts)
    if args.json:
        print(json.dumps(result, indent=2, ensure_ascii=True))
    else:
        print(f"Matrix root: {result['root']}")
        print(f"Found: {', '.join(result['found']) if result['found'] else '(none)'}")
        print(f"Missing: {', '.join(result['missing']) if result['missing'] else '(none)'}")
        print(f"Complete: {result['complete']}")

    if args.require_full_matrix and result["complete"] is False:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
