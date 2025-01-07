from glob import glob

from setuptools import Extension, setup
import os

if __name__ == "__main__":
    setup(
        name="pyawaitable",
        license="MIT",
        version="1.3.0",
        ext_modules=[
            Extension(
                "_pyawaitable",
                glob("./src/_pyawaitable/*.c"),
                include_dirs=["./include/", "./src/pyawaitable/"],
                extra_compile_args=["-g", "-O3" if os.environ.get("PYAWAITABLE_OPTIMIZED") else "-O0"],
            )
        ],
        package_dir={"": "src"},
        packages=["pyawaitable"],
    )
