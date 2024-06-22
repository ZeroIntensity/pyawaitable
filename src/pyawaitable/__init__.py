"""
PyAwaitable - CPython API for asynchronous functions.

Docs: https://awaitable.zintensity.dev/
Source: https://github.com/ZeroIntensity/pyawaitable
"""

from . import abi
from _pyawaitable import _PyAwaitableType  # type: ignore
from typing import Type

__all__ = "PyAwaitable", "include", "abi"

PyAwaitable: Type = _PyAwaitableType

def include() -> str:
    """
    Get the directory containing the `pyawaitable.h` file.
    """
    import os

    return os.path.dirname(__file__)
    
