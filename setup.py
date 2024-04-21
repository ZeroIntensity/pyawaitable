from setuptools import Extension, setup


if __name__ == "__main__":
    setup(
        ext_modules=[
            Extension(
                "_pyawaitable",
                ["./src/awaitable.c"],
                include_dirs=["./include/"],
                define_macros=[("PYAWAITABLE_IS_COMPILING", None)],
                export_symbols=[
                    "awaitable_new",
                    "AwaitableType",
                    "AwaitableGenWrapperType",
                    "awaitable_await",
                    "awaitable_cancel",
                    "awaitable_set_result",
                    "awaitable_save",
                    "awaitable_save_arb",
                    "awaitable_unpack",
                    "awaitable_unpack_arb",
                    "awaitable_init",
                ],
            )
        ],
        headers=["./include/awaitable.h"],
        package_dir={"": "src"},
        packages=["pyawaitable"],
    )
