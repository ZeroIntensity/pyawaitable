"""
PyAwaitable - Call asynchronous code from an extension module.

Docs: https://awaitable.zintensity.dev/
Source: https://github.com/ZeroIntensity/pyawaitable
"""

from typing import Type

from _pyawaitable import _PyAwaitableType  # type: ignore

from . import abi

__all__ = "PyAwaitable", "include", "abi"
__version__ = "1.3.0"

PyAwaitable: Type = _PyAwaitableType


def include() -> str:
    """
    Get the directory containing the `pyawaitable.h` file.
    """
    import os

    return os.path.dirname(__file__)
