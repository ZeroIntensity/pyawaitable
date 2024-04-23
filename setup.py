from setuptools import Extension, setup

if __name__ == "__main__":
    setup(
        ext_modules=[
            Extension(
                "pyawaitable",
                ["./src/awaitable.c"],
                include_dirs=["./include/"],
                define_macros=[("PYAWAITABLE_IS_COMPILING", None)],
            )
        ],
        data_files=[("include", ["./include/awaitable.h"])],
    )
