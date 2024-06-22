from setuptools import Extension, setup
import pyawaitable
import subprocess
if __name__ == "__main__":
    subprocess.run(["pip", "install", "../../"])  # relative paths don't work in pyproject.toml
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
