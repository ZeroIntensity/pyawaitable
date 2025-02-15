import unittest
import _pyawaitable_test
import asyncio
from typing import Any, Callable
from collections.abc import Awaitable

class PyAwaitableTests(unittest.TestCase):
    def test_add_await(self):
        called = False
        async def dummy():
            await asyncio.sleep(0)
            nonlocal called
            called = True

        asyncio.run(_pyawaitable_test.add_await(dummy()))
        self.assertTrue(called)

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
