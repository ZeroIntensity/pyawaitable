"""
PyAwaitable - CPython API for asynchronous functions

If you're importing this from Python, you probably don't need to! PyAwaitable is for use inside C extensions.
Please refer to the docs: https://awaitable.zintensity.dev/
"""
from typing import Any, Generator, NoReturn as __NoReturn, Final as __Final, Any as __Any
from collections.abc import Coroutine as __Coroutine, Iterator as __Iterator

__all__: tuple[str, ...] = (
    "_awaitable",
    "major",
    "minor",
    "micro",
    "_api",

)

class _genwrapper(__Iterator):
    def __init__(self) -> __NoReturn: ...

class _awaitable(__Coroutine):
    def __init__(self) -> __NoReturn: ...
    def __await__(self) -> Generator[_genwrapper, None, _genwrapper]: ...
    __next__ = __await__

major: __Final[int]
minor: __Final[int]
micro: __Final[int]
_api: __Any