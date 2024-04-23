from setuptools import Extension, setup

HEADER_FILES: list[str] = ["./include/awaitable.h"]

if __name__ == "__main__":
    setup(
        # libraries=[LIB],
        # headers=HEADER_FILES,
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
        data_files=[("include", HEADER_FILES)],
    )
