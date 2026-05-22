from __future__ import annotations


def activate_native_commandtab(*args, **kwargs):
    from .bridge import activate_native_commandtab as _activate_native_commandtab

    return _activate_native_commandtab(*args, **kwargs)


def is_native_mode_requested(*args, **kwargs):
    from .bridge import is_native_mode_requested as _is_native_mode_requested

    return _is_native_mode_requested(*args, **kwargs)


__all__ = ["activate_native_commandtab", "is_native_mode_requested"]
