"""
PyAwaitable - CPython API for asynchronous functions.

Docs: https://awaitable.zintensity.dev/
Source: https://github.com/ZeroIntensity/pyawaitable

You probably don't need this module from Python! This module is for use from the C API.
"""

from . import abi
from _pyawaitable import _awaitable
from typing import Type

__all__ = "abi", "Awaitable"

Awaitable: Type = _awaitable
