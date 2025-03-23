import asyncio
from typing import Any, Callable
from collections.abc import Awaitable, Coroutine
import inspect
from pytest import raises, warns

NOT_FOUND = """
The PyAwaitable test package wasn't built!
Please install it with `pip install ./tests`
"""
try:
    import _pyawaitable_test
except ImportError as err:
    raise RuntimeError(NOT_FOUND) from err

def test_awaitable_semantics():
    awaitable = _pyawaitable_test.generic_awaitable(None)
    assert isinstance(awaitable, Awaitable)
    assert isinstance(awaitable, Coroutine)
    # It's not a *native* coroutine
    assert inspect.iscoroutine(awaitable) is False

    with warns(ResourceWarning):
        del awaitable
    
    called = False
    async def dummy():
        await asyncio.sleep(0)
        nonlocal called
        called = True

    assert asyncio.run(_pyawaitable_test.generic_awaitable(dummy())) is None
    assert called is True


async def raising_coroutine() -> None:
    await asyncio.sleep(0)
    raise ZeroDivisionError()

async def dummy_coroutine() -> None:
    await asyncio.sleep(0)


def test_coroutine_propagates_exception():
    awaitable = _pyawaitable_test.coroutine_trampoline(raising_coroutine())
    with raises(ZeroDivisionError):
        asyncio.run(awaitable)


def coro_wrap_call(method: Callable[[Awaitable[Any]], Any], corofunc: Callable[[], Awaitable[Any]]) -> Callable[[], None]:
    def wrapper(*_: Any) -> None:
        method(corofunc())

    return wrapper

def shim_c_function(testfunc: Callable[[], Any]) -> Any:
    def shim():
        testfunc()

    return shim

for method in dir(_pyawaitable_test):
    if not method.startswith("test_"):
        continue

    case: Callable[..., None] = getattr(_pyawaitable_test, method)
    if method.endswith("needs_coro"):
        globals()[method.rstrip("_needs_coro")] = coro_wrap_call(case, dummy_coroutine)
    elif method.endswith("needs_rcoro"):
        globals()[method.rstrip("_needs_rcoro")] = coro_wrap_call(case, raising_coroutine)
    else:
        # Wrap it with a Python function for pytest, because it can't handle C
        # functions for some reason.
        globals()[method] = shim_c_function(case)
