from glob import glob

from setuptools import Extension, setup
import os

DEBUG_SYMBOLS = "/DEBUG" if os.name == "nt" else "-g"
_OPTIMIZED = "/O3" if os.name == "nt" else "-O3"
_NO_OPTIMIZATION = "" if os.name == "nt" else "-O0"
OPTIMIZATION = _OPTIMIZED if os.environ.get("PYAWAITABLE_OPTIMIZED") else _NO_OPTIMIZATION

if __name__ == "__main__":
    setup(
        name="pyawaitable",
        license="MIT",
        version="1.4.0-dev",
        ext_modules=[
            Extension(
                "_pyawaitable",
                glob("./src/_pyawaitable/*.c"),
                include_dirs=["./include/", "./src/pyawaitable/"],
                extra_compile_args=[DEBUG_SYMBOLS, OPTIMIZATION],
            )
        ],
        package_dir={"": "src"},
        packages=["pyawaitable"],
    )
