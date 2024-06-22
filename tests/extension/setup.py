from setuptools import Extension, setup
import sys
import os

# pyproject.toml doesn't let us specify the local pyawaitable
# build as a requirement, so `pip install` when trying to
# import pyawaitable
def include() -> str:
    """
    Get the `include/` directory containing the `pyawaitable.h` file.
    """
    import os
    import site
    import sys

    if sys.prefix != sys.base_prefix:
        # venv, use the exec_prefix
        return os.path.join(sys.exec_prefix, 'include')
    # otherwise, use the USER_BASE
    return os.path.join(site.getuserbase(), 'include')


if __name__ == "__main__":
    setup(
        name="pyawaitable_test",
        ext_modules=[
            Extension(
                "_pyawaitable_test",
                sources=["./test.c", "./a.c"],
                include_dirs=[include()],
            )
        ],
        license="MIT",
    )
