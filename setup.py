from setuptools import Extension, setup
from glob import glob

if __name__ == "__main__":
    setup(
        name="pyawaitable",
        license="MIT",
        ext_modules=[
            Extension(
                "_pyawaitable",
                glob("./src/_pyawaitable/*.c"),
                include_dirs=["./include/"],
                define_macros=[("PYAWAITABLE_IS_COMPILING", None)],
            )
        ],
        package_dir={"": "src"},
        packages=["pyawaitable"],
        data_files=[("include", ["./include/awaitable.h"])],
    )
