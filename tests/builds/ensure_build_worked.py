"""
Script to ensure that an extension module containing PyAwaitable was successfully built.
Intended to be run by Hatch in CI.
"""
import optparse
import sys
import asyncio
import traceback
import importlib
import site
import os
from typing import TypeVar

T = TypeVar("T")

def not_none(value: T | None) -> T:
    assert value is not None, "got None where None shouldn't be"
    return value

def debug_directory(what: str, path: str) -> None:
    print(f"{what}: {path}", file=sys.stderr)
    print(f"  Contents of {what}:", file=sys.stderr)
    for root, _, files in os.walk(path):
        for file in files:
            print(f"  {what}: {os.path.join(root, file)}", file=sys.stderr)

def debug_import_error(error: ImportError) -> None:
    #debug_directory("PyAwaitable Include", pyawaitable.include())
    debug_directory("User Site", not_none(site.USER_SITE))
    debug_directory("User Base", not_none(site.USER_BASE))
    print(error)

def main():
    parser = optparse.OptionParser()
    _, args = parser.parse_args()

    if not args:
        parser.error("Expected one argument.")

    mod = args[0]

    called = False
    async def dummy():
        await asyncio.sleep(0)
        nonlocal called
        called = True

    try:
        asyncio.run(importlib.import_module(mod).async_function(dummy()))
    except BaseException as err:
        traceback.print_exc()
        if isinstance(err, ImportError):
            debug_import_error(err)
            print("Failed to import the module!", file=sys.stderr)
        else:
            print("Build failed!", file=sys.stderr)
        sys.exit(1)
    
    if not called:
        print("Build doesn't work!", file=sys.stderr)
        sys.exit(1)
    
    print("Success!")

if __name__ == "__main__":
    main()
