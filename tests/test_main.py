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
    
async def raising_coroutine():
    await asyncio.sleep(0)
    raise ZeroDivisionError()


def test_coroutine_propagates_exception():
    awaitable = _pyawaitable_test.coroutine_trampoline(raising_coroutine())
    with raises(ZeroDivisionError):
        asyncio.run(awaitable)


def coro_wrap_call(method: Callable[[Awaitable[Any]], Any]) -> Callable[[], None]:
    def wrapper(*_: Any):
        async def dummy():
            await asyncio.sleep(0)

        method(dummy())

    return wrapper

for method in dir(_pyawaitable_test):
    if not method.startswith("test_"):
        continue

    case: Callable[..., None] = getattr(_pyawaitable_test, method)
    if method.endswith("needs_coro"):
        globals()[method.rstrip("_needs_coro")] = coro_wrap_call(case)
    else:
        # Wrap it with a Python function for pytest, but we have to do
        # this extra shim to keep a reference to the right call.
        # I'm tired right now, so if there's an easier way to do this, feel
        # free to submit a PR.
        def _wrapped(testfunc: Callable[..., Any]) -> Any:
            return lambda: testfunc()
        globals()[method] = _wrapped(case)
