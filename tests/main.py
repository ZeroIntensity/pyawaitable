import unittest
import _pyawaitable_test

class PyAwaitableTests(unittest.TestCase):
    def setUp(self) -> None:
        for method in dir(_pyawaitable_test):
            if method.startswith("test_"):
                setattr(self, method, getattr(_pyawaitable_test, method))
