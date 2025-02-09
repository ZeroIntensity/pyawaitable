"""
PyAwaitable Vendoring Script
"""

import os
from pathlib import Path
from typing import Callable, TextIO, ParamSpec, TypeVar
import re
from contextlib import contextmanager
import functools
import textwrap

try:
    from hatchling.builders.hooks.plugin.interface import BuildHookInterface
except ImportError:

    class BuildHookInterface:
        pass


DIST_PATH: str = "src/pyawaitable/pyawaitable.h"
HEADER_FILES: list[str] = [
    "optimize.h",
    "dist.h",
    "array.h",
    "backport.h",
    "coro.h",
    "awaitableobject.h",
    "genwrapper.h",
    "values.h",
    "with.h",
    "init.h",
]
SOURCE_FILES: list[Path] = [
    Path("./src/_pyawaitable/array.c"),
    Path("./src/_pyawaitable/coro.c"),
    Path("./src/_pyawaitable/awaitable.c"),
    Path("./src/_pyawaitable/genwrapper.c"),
    Path("./src/_pyawaitable/values.c"),
    Path("./src/_pyawaitable/with.c"),
    Path("./src/_pyawaitable/init.c"),
]

INCLUDE_REGEX = re.compile(r"#include <(.+)>")
FUNCTION_REGEX = re.compile(r"(.+)\(.*\).*")
INTERNAL_FUNCTION_REGEX = re.compile(r"_PyAwaitable_INTERNAL\(.+\)\n(.+)\(.*\).*")
INTERNAL_DATA_REGEX = re.compile(r"_PyAwaitable_INTERNAL_DATA\(.+\) (.+)")
EXPLICIT_REGEX = re.compile(r".*_PyAwaitable_MANGLE\((.+)\).*")
NO_EXPLICIT_REGEX = re.compile(r".*_PyAwaitable_NO_MANGLE\((.+)\).*")
DEFINE_REGEX = re.compile(r" *# *define *(\w+)(\(.*\))?.*")

HEADER_GUARD = """
#if !defined(PYAWAITABLE_VENDOR_H) && !defined(Py_LIMITED_API)
#define PYAWAITABLE_VENDOR_H
#define _PYAWAITABLE_VENDOR
"""
HEADER = (
    lambda version: f"""\
/*
 * PyAwaitable - Autogenerated distribution copy of version {version}
 *
 * Docs: https://awaitable.zintensity.dev
 * Source: https://github.com/ZeroIntensity/pyawaitable
 */
"""
)
MANGLED = "__PyAwaitable_Mangled_"

_LOG_NEST = 0


@contextmanager
def logging_context():
    global _LOG_NEST
    assert _LOG_NEST >= 0
    try:
        _LOG_NEST += 1
        yield
    finally:
        _LOG_NEST -= 1


T = TypeVar("T")
P = ParamSpec("P")


def new_context(message: str) -> Callable[[Callable[P, T]], Callable[P, T]]:
    def decorator_factory(func: Callable[P, T]) -> Callable[P, T]:
        @functools.wraps(func)
        def decorator(*args: P.args, **kwargs: P.kwargs):
            log(message)
            with logging_context():
                return func(*args, **kwargs)

        return decorator

    return decorator_factory


def log(*text: str) -> None:
    indent = "    " * _LOG_NEST
    print(indent + " ".join(text))


def write(fp: TextIO, value: str) -> None:
    fp.write(value + "\n")
    log(f"Wrote {len(value.encode('utf-8'))} bytes to {fp.name}")


@new_context("Finding includes...")
def find_includes(lines: list[str], includes: set[str]) -> None:
    for line in lines.copy():
        match = INCLUDE_REGEX.match(line)
        if match:
            lines.remove(line)
            include = match.group(1)
            if include.startswith("pyawaitable/"):
                # Won't exist in the vendor
                continue

            includes.add(include)


@new_context("Finding source file macros...")
def find_defines(lines: list[str], defines: set[str]) -> None:
    for line in lines:
        match = DEFINE_REGEX.match(line)
        if not match:
            continue

        defines.add(match.group(1))


def filter_name(name: str) -> bool:
    return name.startswith("__")


@new_context("Processing explicitly marked name...")
def mangle_explicit(changed_names: dict[str, str], line: str) -> None:
    explicit = EXPLICIT_REGEX.match(line)
    if explicit is None:
        raise RuntimeError(f"{line} does not follow _PyAwaitable_MANGLE correctly")

    name = explicit.group(1)
    if filter_name(name):
        return
    changed_names[name] = MANGLED + name
    log(f"Marked {name} for mangling")


@new_context("Processing _PyAwaitable_INTERNAL function...")
def mangle_internal(
    changed_names: dict[str, str], lines: list[str], index: int
) -> None:
    try:
        func_def = INTERNAL_FUNCTION_REGEX.match(lines[index] + "\n" + lines[index + 1])
    except IndexError:
        return

    if func_def is None:
        return

    name = func_def.group(1)
    if filter_name(name):
        return
    changed_names[name] = "__PyAwaitable_Internal_" + name
    log(f"Marked {name} for mangling")


@new_context("Processing internal data...")
def mangle_internal_data(changed_names: dict[str, str], line: str) -> None:
    internal_data = INTERNAL_DATA_REGEX.match(line)
    if internal_data is None:
        raise RuntimeError(
            f"{line} does not follow _PyAwaitable_INTERNAL_DATA correctly"
        )

    name = internal_data.group(1)
    changed_names[name] = "__PyAwaitable_InternalData_" + name
    log(f"Marked {name} for mangling")


@new_context("Processing static function...")
def mangle_static(changed_names: dict[str, str], lines: list[str], index: int) -> None:
    try:
        line = lines[index + 1]
    except IndexError:
        return

    if NO_EXPLICIT_REGEX.match(line) is not None:
        return

    func_def = FUNCTION_REGEX.match(line)

    if func_def is None:
        return

    name = func_def.group(1)
    if filter_name(name):
        return
    changed_names[name] = "__PyAwaitable_Static_" + name
    log(f"Marked {name} for mangling")


@new_context("Calculating mangled names...")
def mangle_names(changed_names: dict[str, str], lines: list[str]) -> None:
    for index, line in enumerate(lines):
        if line.startswith("#define"):
            continue

        if "_PyAwaitable_MANGLE" in line:
            mangle_explicit(changed_names, line)
        elif "_PyAwaitable_INTERNAL" in line:
            mangle_internal(changed_names, lines, index)
        elif "_PyAwaitable_INTERNAL_DATA" in line:
            mangle_internal_data(changed_names, line)
        elif line.startswith("static"):
            mangle_static(changed_names, lines, index)


def orderize_mangled(changed_names: dict[str, str]) -> dict[str, str]:
    result: dict[str, str] = {}
    orders: list[tuple[str, int]] = []

    for name in changed_names.keys():
        # Count how many times other keys go into name
        amount = 0
        for second_name in changed_names.keys():
            if second_name in name:
                amount += 1

        orders.append((name, amount))

    orders.sort(key=lambda item: item[1])
    for index, data in enumerate(orders.copy()):
        name, amount = data
        if changed_names[name].startswith(MANGLED):
            # Always do explicit mangles last
            orders.insert(0, orders.pop(index))

    for name, amount in reversed(orders):
        result[name] = changed_names[name]

    return result


DOUBLE_MANGLE = "__PyAwaitable_Mangled___PyAwaitable_Mangled_"


def clean_mangled(text: str) -> str:
    return text.replace(DOUBLE_MANGLE, MANGLED)


def process_files(fp: TextIO) -> None:
    includes = set[str]()
    to_write: list[str] = []
    log("Processing header files...")
    changed_names: dict[str, str] = {}
    source_macros: set[str] = set()

    with logging_context():
        for header_file in HEADER_FILES:
            header_file = "include/pyawaitable" / Path(header_file)
            log(f"Processing {header_file}")
            lines: list[str] = header_file.read_text(encoding="utf-8").split("\n")
            find_includes(lines, includes)
            mangle_names(changed_names, lines)
            to_write.append("\n".join(lines))

    log("Processing source files...")
    with logging_context():
        for source_file in SOURCE_FILES:
            lines: list[str] = source_file.read_text(encoding="utf-8").split("\n")
            log(f"Processing {source_file}")
            find_includes(lines, includes)
            mangle_names(changed_names, lines)
            find_defines(lines, source_macros)
            to_write.append("\n".join(lines))

    log("Writing macros...")
    with logging_context():
        for include in includes:
            assert not include.startswith(
                "pyawaitable/"
            ), "found pyawaitable headers somehow"
            write(fp, f"#include <{include}>")

    log("Writing mangled names...")
    with logging_context():
        for name, new_name in orderize_mangled(changed_names).items():
            for index, line in enumerate(to_write):
                to_write[index] = clean_mangled(line.replace(name, new_name))

        for line in to_write:
            write(fp, line)

    log("Writing macro cleanup...")
    with logging_context():
        for define in defines:
            write(fp, f"#undef {define}")


def main(version: str) -> None:
    dist = Path(DIST_PATH)
    if dist.exists():
        log(f"{dist} already exists, removing it...")
        os.remove(dist)
    log("Creating vendored copy of pyawaitable...")

    major, minor, micro = version.split(".", maxsplit=3)
    version_text = textwrap.dedent(
        f"""
    #define PyAwaitable_MAJOR_VERSION {major}
    #define PyAwaitable_MINOR_VERSION {minor}
    #define PyAwaitable_MICRO_VERSION {micro}
    #define PyAwaitable_PATCH_VERSION PyAwaitable_MICRO_VERSION
    #define PyAwaitable_MAGIC_NUMBER {major}{minor}{micro}
    """
    )

    with open(dist, "w", encoding="utf-8") as f:
        with logging_context():
            write(f, HEADER(version) + HEADER_GUARD + version_text)
            process_files(f)
            write(
                f,
                """#else
#error "the limited API cannot be used with pyawaitable"
#endif /* PYAWAITABLE_VENDOR_H */""",
            )

    log(f"Created PyAwaitable distribution at {dist}")


class CustomBuildHook(BuildHookInterface):
    PLUGIN_NAME = "PyAwaitable Build"

    def clean(self, _: list[str]) -> None:
        dist = Path(DIST_PATH)
        if dist.exists():
            os.remove(dist)

    def initialize(self, _: str, build_data: dict) -> None:
        self.clean([])
        main(self.metadata.version)
        build_data["force_include"][DIST_PATH] = DIST_PATH


if __name__ == "__main__":
    from src.pyawaitable import __version__

    main(__version__)
