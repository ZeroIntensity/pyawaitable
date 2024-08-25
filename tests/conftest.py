import ctypes
import functools
import inspect
import platform
import sys
from typing import Any, Callable

import pytest

from pyawaitable.bindings import awaitcallback, awaitcallback_err

try:
    import _pyawaitable_test
except ImportError:
    pytest.exit(
        "PyAwaitable testing package has not been build! (Hint: pip install tests/extension --no-build-isolation)",  # noqa
        returncode=-1,
    )

ITERATIONS: int = 10000
LEAK_LIMIT: str = "50 KB"

raising_callback = ctypes.cast(
    _pyawaitable_test.raising_callback, awaitcallback
)
raising_err_callback = ctypes.cast(
    _pyawaitable_test.raising_err_callback, awaitcallback_err
)


def pytest_addoption(parser: Any) -> None:
    parser.addoption("--enable-leak-tracking", action="store_true")


def limit_leaks(func: Callable):
    if "--enable-leak-tracking" not in sys.argv:
        return func

    if platform.system() != "Windows":
        if not inspect.iscoroutinefunction(func):

            @functools.wraps(func)
            def wrapper(*args, **kwargs):  # type: ignore
                for _ in range(ITERATIONS):
                    func(*args, **kwargs)

        else:

            @functools.wraps(func)
            async def wrapper(*args, **kwargs):
                for _ in range(ITERATIONS):
                    await func(*args, **kwargs)

            wrapper = pytest.mark.asyncio(wrapper)

        return pytest.mark.limit_leaks(LEAK_LIMIT)(wrapper)
    else:
        return func
