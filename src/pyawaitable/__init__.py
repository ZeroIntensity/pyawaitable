"""
PyAwaitable - CPython API for asynchronous functions.

Docs: https://awaitable.zintensity.dev/
Source: https://github.com/ZeroIntensity/pyawaitable

You probably don't need this module from Python! This module is for use from the C API.
"""

from . import abi
from _pyawaitable import _PyAwaitableType  # type: ignore
from typing import Type

__all__ = "PyAwaitable", "include", "abi"

PyAwaitable: Type = _PyAwaitableType

def include() -> str:
    """
    Get the `include/` directory containing the `pyawaitable.h` file.
    """
    import os
    import site
    import sys

    if sys.prefix != sys.base_prefix:
        # venv, use the exec_prefix
        return os.path.join(sys.exec_prefix, 'include')
    # otherwise, use the USER_BASE
    return os.path.join(site.getuserbase(), 'include')
    
