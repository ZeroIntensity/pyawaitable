from setuptools import Extension, setup
import site
import os

# pyproject.toml doesn't let us specify the local pyawaitable
# build as a requirement, so `pip install` fails when trying to
# import pyawaitable

if __name__ == "__main__":
    setup(
        name="pyawaitable_test",
        ext_modules=[
            Extension(
                "_pyawaitable_test",
                sources=["./test.c", "./a.c"],
                include_dirs=[os.path.join(site.getusersitepackages(), "pyawaitable")],
            )
        ],
        license="MIT",
    )
