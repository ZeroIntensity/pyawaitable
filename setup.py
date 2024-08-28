from glob import glob

from setuptools import Extension, setup

if __name__ == "__main__":
    setup(
        name="pyawaitable",
        license="MIT",
        version="1.3.0-dev",
        ext_modules=[
            Extension(
                "_pyawaitable",
                glob("./src/_pyawaitable/*.c"),
                include_dirs=["./include/", "./src/pyawaitable/"],
                extra_compile_args=["-g", "-O3"],
            )
        ],
        package_dir={"": "src"},
        packages=["pyawaitable"],
    )
