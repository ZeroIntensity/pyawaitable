"""
PyAwaitable - CPython API for asynchronous functions.

You only need this module in Python for getting the include path in a build script.
Otherwise, all usage of this module is in C (or some other native language)!
"""

from pathlib import Path

__all__ = ("include",)

__version__ = "1.0.0"
__author__ = "ZeroIntensity <zintensitydev@gmail.com>"
__authors__ = (__author__,)
__license__ = "MIT"


def include() -> str:
    """Get the absolute include path for PyAwaitable."""
    path = Path(__file__).absolute()
    package = path.parent

    if package.name != "pyawaitable":
        raise FileNotFoundError(
            f"expected pyawaitable, but instead inside {package}"
        )

    site = package.parent

    if site.name != "site-packages":
        raise FileNotFoundError(f"expected site-packages, but instead inside {site}")
    
    include = site.parent.parent.parent / "include"
    target: Path | None = None

    for i in include.rglob("*"):
        if (i.name == "awaitable.h") and (i.parent.name == "pyawaitable"):
            target = i.parent

    if not target:
        raise FileNotFoundError(f"could not find pyawaitable include path at {path}")

    return str(target)
