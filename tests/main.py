import unittest
import _pyawaitable_test

class PyAwaitableTests(unittest.TestCase):
    pass

for method in dir(_pyawaitable_test):
    if method.startswith("test_"):
        setattr(PyAwaitableTests, method, getattr(_pyawaitable_test, method))

if __name__ == "__main__":
    unittest.main()