import pyawaitable
import ctypes
from ctypes import pythonapi
import pytest
import asyncio
import platform
from typing import Callable

tstate_init = pythonapi.PyGILState_Ensure
tstate_init.restype = ctypes.c_long
tstate_init.argtypes = ()

tstate_release = pythonapi.PyGILState_Release
tstate_release.argtypes = ctypes.c_long,

get_pointer = pythonapi.PyCapsule_GetPointer
get_pointer.argtypes = (ctypes.py_object, ctypes.c_void_p)
get_pointer.restype = ctypes.c_void_p

awaitcallback = ctypes.CFUNCTYPE(ctypes.py_object, ctypes.py_object)
awaitcallback_err = awaitcallback

# Initialize API array
ptr = get_pointer(pyawaitable._api, None)
api = (ctypes.c_void_p * 10).from_address(ptr)

# Types
AwaitableType = ctypes.cast(api[0], ctypes.py_object).value
AwaitableGenWrapperType = ctypes.cast(api[1], ctypes.py_object).value

# API Functions
awaitable_new = ctypes.cast(api[2], ctypes.CFUNCTYPE(ctypes.py_object))
awaitable_await = ctypes.cast(
    api[3],
    ctypes.CFUNCTYPE(
        ctypes.c_int,
        ctypes.py_object,
        ctypes.py_object,
        awaitcallback,
        awaitcallback_err,
    ),
)


def test_api_types():
    assert AwaitableType is pyawaitable._awaitable
    assert AwaitableGenWrapperType is pyawaitable._genwrapper

def limit_leaks(memstring: str) -> None:
    def decorator(func: Callable):
        if platform().system != "Windows":
            return pytest.mark.limit_leaks(memstring)(func)
        else:
            return func
    return decorator

@limit_leaks("5 KB")
@pytest.mark.asyncio
async def test_new():
    assert isinstance(awaitable_new(), pyawaitable._awaitable)
    await asyncio.create_task(awaitable_new())
    await awaitable_new()


@limit_leaks("5 KB")
@pytest.mark.asyncio
async def test_await():
    event = asyncio.Event()

    async def coro():
        event.set()

    awaitable = awaitable_new()
    awaitable_await(awaitable, coro(), awaitcallback(0), awaitcallback_err(0))
    await awaitable
    assert event.is_set()


@limit_leaks("5 KB")
@pytest.mark.asyncio
async def test_await_cb():
    awaitable = awaitable_new()

    async def coro(value: int):
        return value * 2

    @awaitcallback
    def cb(awaitable_inner: AwaitableType, result: int) -> int:
        assert awaitable_inner is awaitable
        assert result == 42
        return 0

    awaitable_await(awaitable, coro(21), cb, awaitcallback_err(0))
