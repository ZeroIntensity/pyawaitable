from setuptools import Extension, setup

if __name__ == "__main__":
    setup(
        ext_modules=[
            Extension(
                "_pyawaitable",
                ["./src/awaitable.c"],
                include_dirs=["./include/"],
                define_macros=[("PYAWAITABLE_IS_COMPILING", None)],
            )
        ],
        package_dir={"": "src"},
        packages=["pyawaitable"],
        data_files=[("include", ["./include/awaitable.h"])],
    )
