"""
PyAwaitable - Call asynchronous code from an extension module.

It's unlikely that you want to import this module from Python, other than
for use in setuptools

Docs: https://awaitable.zintensity.dev/
Source: https://github.com/ZeroIntensity/pyawaitable
"""

__all__ = "include",
__version__ = "2.0.0"
__author__ = "Peter Bierma"

def include() -> str:
    """
    Get the directory containing the `pyawaitable.h` file.
    """
    import os

    return os.path.dirname(__file__)
