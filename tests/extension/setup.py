from setuptools import Extension, setup
import subprocess
import site
import os

if __name__ == "__main__":
    subprocess.run(["pip", "install", "../../"])  # relative paths don't work in pyproject.toml

    setup(
        name="pyawaitable_test",
        ext_modules=[
            Extension(
                "_pyawaitable_test",
                sources=["./test.c", "./a.c"],
                include_dirs=[os.path.join(site.getusersitepackages(), "include")],
            )
        ],
        license="MIT",
    )
