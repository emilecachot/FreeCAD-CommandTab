from __future__ import annotations

from pathlib import Path

from tools import verify_native_matrix


def _touch_binary(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(b"binary")


def test_verify_matrix_detects_missing_entries(tmp_path: Path) -> None:
    _touch_binary(tmp_path / "linux" / "qt6" / "libfreecad_commandtab_native_backend.so")
    result = verify_native_matrix.verify_matrix(tmp_path)
    assert result["complete"] is False
    assert "linux/qt6" in result["found"]
    assert "windows/qt5" in result["missing"]


def test_verify_matrix_complete(tmp_path: Path) -> None:
    for platform, filename in [
        ("linux", "libfreecad_commandtab_native_backend.so"),
        ("windows", "freecad_commandtab_native_backend.dll"),
        ("macos", "libfreecad_commandtab_native_backend.dylib"),
    ]:
        for qt in ["qt5", "qt6"]:
            _touch_binary(tmp_path / platform / qt / filename)

    result = verify_native_matrix.verify_matrix(tmp_path)
    assert result["complete"] is True
    assert result["missing"] == []


def test_verify_matrix_can_scope_linux_and_macos_only(tmp_path: Path) -> None:
    _touch_binary(tmp_path / "linux" / "qt6" / "libfreecad_commandtab_native_backend.so")
    _touch_binary(tmp_path / "macos" / "qt6" / "libfreecad_commandtab_native_backend.dylib")

    result = verify_native_matrix.verify_matrix(
        tmp_path,
        platforms=("linux", "macos"),
        qts=("qt6",),
    )

    assert result["complete"] is True
    assert result["found"] == ["linux/qt6", "macos/qt6"]
    assert result["missing"] == []
