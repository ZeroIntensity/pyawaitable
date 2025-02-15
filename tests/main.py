import unittest
import _pyawaitable_test
import asyncio
from typing import Any, Callable
from collections.abc import Awaitable, Coroutine
import inspect


class PyAwaitableTests(unittest.TestCase):
    def test_awaitable_semantics(self):
        awaitable = _pyawaitable_test.generic_awaitable(None)
        self.assertIsInstance(awaitable, Awaitable)
        self.assertIsInstance(awaitable, Coroutine)
        # It's not a *native* coroutine
        self.assertFalse(inspect.iscoroutine(awaitable))

        with self.assertWarns(ResourceWarning):
            del awaitable

def coro_wrap_call(method: Callable[[Awaitable[Any]], Any]) -> Callable[[], None]:
    def wrapper(*_: Any):
        async def dummy():
            await asyncio.sleep(0)

        method(dummy())

    return wrapper

for method in dir(_pyawaitable_test):
    if method.startswith("test_"):
        test_case = getattr(_pyawaitable_test, method)
        if method.endswith("needs_coro"):
            setattr(PyAwaitableTests, method.rstrip("_needs_coro"), coro_wrap_call(test_case))
        else:
            setattr(PyAwaitableTests, method, test_case)

if __name__ == "__main__":
    unittest.main()
