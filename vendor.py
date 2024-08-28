import os
import shutil
from pathlib import Path
from typing import TextIO

from pyawaitable import __version__


def _write(fp: TextIO, value: str) -> None:
    fp.write(value)
    print(f"Wrote {len(value.encode('utf-8'))} bytes to {fp.name}")


def _header_file(text: str) -> str:
    lines = text.replace("_impl", "").split("\n")

    # Remove header guards and trailing newlines
    for i in (0, 0, -1, -1):
        lines.pop(i)

    return "\n".join([i for i in lines if not i.startswith("#include")])


def _source_file(fp: TextIO, file: Path) -> None:
    text: str = file.read_text(encoding="utf-8").replace("_impl", "")
    lines = text.split("\n")
    lines.insert(0, f"/* Vendor of {file} */")
    _write(fp, "\n".join([i for i in lines if not i.startswith("#include")]))


def main():
    dist = Path("./pyawaitable-vendor")
    if dist.exists():
        print("./pyawaitable-vendor already exists, removing it...")
        shutil.rmtree(dist)
    print("Creating vendored copy of pyawaitable in ./pyawaitable-vendor...")
    os.mkdir(dist)

    with open(dist / "pyawaitable.h", "w", encoding="utf-8") as f:
        _write(
            f,
            """#ifndef PYAWAITABLE_VENDOR_H
#define PYAWAITABLE_VENDOR_H

#include <Python.h>
#include <stdbool.h>
#include <stdlib.h>
""",
        )
        for path in [i for i in Path("./include/pyawaitable").iterdir()] + [
            Path("./include/vendor.h")
        ]:
            _write(f, _header_file(path.read_text(encoding="utf-8")))

        _write(f, "\n#endif")

    with open(dist / "pyawaitable.c", "w", encoding="utf-8") as f:
        f.write(
            f"""/*
 * PyAwaitable - Vendored copy of version {__version__}
 *
 * Docs: https://awaitable.zintensity.dev
 * Source: https://github.com/ZeroIntensity/pyawaitable
 */

#include "pyawaitable.h"

PyTypeObject _PyAwaitableGenWrapperType; // Forward declaration
"""
        )
        for source_file in Path("./src/_pyawaitable/").iterdir():
            if source_file.name == "mod.c":
                continue

            _source_file(f, source_file)


if __name__ == "__main__":
    main()
