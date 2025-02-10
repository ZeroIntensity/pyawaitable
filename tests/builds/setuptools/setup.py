import pyawaitable
from setuptools import setup, Extension
from glob import glob

if __name__ == "__main__":
    setup(
        ext_modules=[
            Extension(
                "_pyawaitable_test",
                glob("../../*.c"),
                include_dirs=[pyawaitable.include()],
            )
        ]
    )
