import unittest
import _pyawaitable_test
import asyncio

class PyAwaitableTests(unittest.TestCase):
    def test_add_await(self):
        called = False
        async def dummy():
            await asyncio.sleep(0)
            nonlocal called
            called = True

        asyncio.run(_pyawaitable_test.add_await(dummy()))
        self.assertTrue(called)

for method in dir(_pyawaitable_test):
    if method.startswith("test_"):
        setattr(PyAwaitableTests, method, getattr(_pyawaitable_test, method))

if __name__ == "__main__":
    unittest.main()
