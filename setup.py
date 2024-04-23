from setuptools import Extension, setup

HEADER_FILES: list[str] = ["./include/awaitable.h"]

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
        data_files=[("include", HEADER_FILES)],
    )
