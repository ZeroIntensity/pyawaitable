from setuptools import Extension, setup
import pyawaitable

if __name__ == "__main__":
    setup(
        name="pyawaitable_test",
        ext_modules=[
            Extension(
                "_pyawaitable_test",
                sources=["./test.c", "./a.c"],
                include_dirs=[pyawaitable.include()],
            )
        ],
        license="MIT",
    )
