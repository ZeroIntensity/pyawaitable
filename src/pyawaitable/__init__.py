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
    from pathlib import Path

    directory = Path(__file__).parent
    if "pyawaitable.h" not in os.listdir(directory):
        raise RuntimeError("pyawaitable.h wasn't found! Are you sure your installation is correct?")

    return str(directory.absolute())
