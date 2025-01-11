import pyawaitable
from setuptools import setup, Extension

if __name__ == "__main__":
    setup(
        ext_modules=[
            Extension(
                "_pyawaitable_test",
                ["../module.c"],
                include_dirs=[pyawaitable.include()],
            )
        ]
    )
