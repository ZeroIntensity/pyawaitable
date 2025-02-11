from setuptools import setup, Extension
from pathlib import Path
from glob import glob

NOT_FOUND = """pyawaitable.h wasn't found! It probably wasn't built.

To build a working copy, you can either install the package
locally (via `pip install .`), or by executing the `hatch_build.py` file.
"""

def find_local_pyawaitable() -> Path:
    """Find the directory containing the local copy of pyawaitable.h"""
    top_level = Path(__file__).parent.parent
    source = top_level / "src" / "pyawaitable"
    if not (source / "pyawaitable.h").exists():
        raise RuntimeError(NOT_FOUND)
    
    return str(source.absolute())

if __name__ == "__main__":
    setup(
        ext_modules=[
            Extension(
                "_pyawaitable_test",
                glob("*.c"),
                include_dirs=[find_local_pyawaitable()],
            )
        ]
    )
