from setuptools import Extension, setup
from glob import glob

if __name__ == "__main__":
    setup(
        name="pyawaitable",
        license="MIT",
        version = "1.0.0",
        ext_modules=[
            Extension(
                "_pyawaitable",
                glob("./src/_pyawaitable/*.c"),
                include_dirs=["./include/", "./src/pyawaitable/"],
                extra_compile_args=["-g", "-O3"]
            )
        ],
        package_dir={"": "src"},
        packages=["pyawaitable"],
    )
