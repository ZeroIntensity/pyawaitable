import pyawaitable
import ctypes
from ctypes import pythonapi
import pytest
import asyncio
import platform
from typing import Callable, Any
from typing_extensions import Self

get_pointer = pythonapi.PyCapsule_GetPointer
get_pointer.argtypes = (ctypes.py_object, ctypes.c_void_p)
get_pointer.restype = ctypes.c_void_p

get_name = pythonapi.PyCapsule_GetName
get_name.argtypes = (ctypes.py_object,)
get_name.restype = ctypes.c_char_p

set_err_str = pythonapi.PyErr_SetString
set_err_str.argtypes = (ctypes.py_object, ctypes.c_char_p)
set_err_str.restype = None

class PyABI(ctypes.Structure):
    @classmethod
    def from_capsule(cls, capsule: Any) -> Self:
        # Assume that argtypes and restype have been properly set
        capsule_name = get_name(capsule)
        abi = ctypes.cast(
            get_pointer(capsule, capsule_name),
            ctypes.POINTER(cls)
        )

        return abi.contents

    def __init_subclass__(cls, **kwargs) -> None:
        super().__init_subclass__(**kwargs)
        # Assume that _fields_ is a list
        cls._fields_.insert(0, ("size", ctypes.c_ssize_t))

    def __getattribute__(self, name: str) -> Any:
        size = super().__getattribute__("size")
        offset = getattr(type(self), name).offset

        if size <= offset:
            raise ValueError(f"{name!r} is not available on this ABI version")

        attr = super().__getattribute__(name)
        return attr

awaitcallback = ctypes.PYFUNCTYPE(ctypes.c_int, ctypes.py_object, ctypes.py_object)
awaitcallback_err = awaitcallback

class AwaitableABI(PyABI):
    _fields_ = [
        ("awaitable_new", ctypes.PYFUNCTYPE(ctypes.py_object)),
        (
            "awaitable_await",
            ctypes.PYFUNCTYPE(
                ctypes.c_int,
                ctypes.py_object,
                ctypes.py_object,
                awaitcallback,
                awaitcallback_err,
            )
        ),
        ("awaitable_cancel", ctypes.PYFUNCTYPE(None, ctypes.py_object)),
        ("awaitable_set_result", ctypes.PYFUNCTYPE(ctypes.c_int, ctypes.py_object, ctypes.py_object)),
        ("awaitable_save", ctypes.PYFUNCTYPE(ctypes.c_int, ctypes.py_object, ctypes.c_ssize_t)),
        ("awaitable_save_arb", ctypes.PYFUNCTYPE(ctypes.c_int, ctypes.py_object, ctypes.c_ssize_t)),
        ("awaitable_unpack", ctypes.PYFUNCTYPE(ctypes.c_int, ctypes.py_object)),
        ("awaitable_unpack_arb", ctypes.PYFUNCTYPE(ctypes.c_int, ctypes.py_object)),
    ]


abi = AwaitableABI.from_capsule(pyawaitable.abi.v1)

def limit_leaks(memstring: str):
    def decorator(func: Callable):
        if platform.system() != "Windows":
            func = pytest.mark.limit_leaks(memstring)(func)
            return func
        else:
            return func
    return decorator


@limit_leaks("5 KB")
@pytest.mark.asyncio
async def test_new():
    assert isinstance(abi.awaitable_new(), pyawaitable._awaitable)
    await asyncio.create_task(abi.awaitable_new())
    await abi.awaitable_new()


@limit_leaks("5 KB")
@pytest.mark.asyncio
async def test_await():
    event = asyncio.Event()

    async def coro():
        event.set()

    awaitable = abi.awaitable_new()
    abi.awaitable_await(awaitable, coro(), awaitcallback(0), awaitcallback_err(0))
    await awaitable
    assert event.is_set()


@limit_leaks("5 KB")
@pytest.mark.asyncio
async def test_await_cb():
    awaitable = abi.awaitable_new()

    async def coro(value: int):
        return value * 2

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.Awaitable, result: int) -> int:
        assert awaitable_inner is awaitable
        assert result == 42
        return 0

    abi.awaitable_await(awaitable, coro(21), cb, awaitcallback_err(0))
    await awaitable

@limit_leaks("5 KB")
@pytest.mark.asyncio
async def test_await_cb_err():
    awaitable = abi.awaitable_new()

    async def coro_raise() -> float:
        return 0 / 0

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.Awaitable, result: float) -> int:
        set_err_str(RuntimeError, b"no good!")

    @awaitcallback_err
    def cb_err(awaitable_inner: pyawaitable.Awaitable, err: Exception) -> int:
        assert isinstance(err, ZeroDivisionError)
        return 0
    
    abi.awaitable_await(awaitable, coro_raise(), cb, cb_err)
    await awaitable

@limit_leaks("5 KB")
@pytest.mark.asyncio
async def test_await_cb_err_cb():
    awaitable = abi.awaitable_new()

    async def coro() -> int:
        return 42

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.Awaitable, result: float) -> int:
        set_err_str(RuntimeError, b"test")
        return -1

    @awaitcallback_err
    def cb_err(awaitable_inner: pyawaitable.Awaitable, err: Exception) -> int:
        assert isinstance(err, RuntimeError)
        assert str(err) == "test"
        return 0

    abi.awaitable_await(awaitable, coro(), cb, cb_err)
    await awaitable

@limit_leaks("5 KB")
@pytest.mark.asyncio
async def test_await_cb_noerr():
    awaitable = abi.awaitable_new()

    async def coro() -> int:
        return 42
    
    @awaitcallback
    def cb(awaitable_inner: pyawaitable.Awaitable, result: int) -> int:
        return -1
    
    abi.awaitable_await(awaitable, coro(), cb, awaitcallback_err(0))

    with pytest.raises(SystemError):
        await awaitable

"""
TODO: figure out how to raise from ctypes callback

@limit_leaks("5 KB")
@pytest.mark.asyncio
async def test_await_cb_err_restore():
    awaitable = abi.awaitable_new()
    event = asyncio.Event()

    async def coro() -> int:
        return 42
    
    @awaitcallback
    def cb(awaitable_inner: pyawaitable.Awaitable, result: int) -> int:
        set_err_str(RuntimeError, "test")

    @awaitcallback_err
    def cb_err(awaitable_inner: pyawaitable.Awaitable, err: Exception) -> int:
        assert str(err) == "test"
        event.set()
        return -1

    abi.awaitable_await(awaitable, coro(), cb, cb_err)

    with pytest.raises(RuntimeError):
        await awaitable

    assert event.is_set()

@limit_leaks("5 KB")
@pytest.mark.asyncio
async def test_await_cb_err_norestore():
    awaitable = abi.awaitable_new()
    event = asyncio.Event()

    async def coro() -> int:
        return 42
    
    @awaitcallback
    def cb(awaitable_inner: pyawaitable.Awaitable, result: int) -> int:
        set_err_str(RuntimeError, b"test")
        return -1

    @awaitcallback_err
    def cb_err(awaitable_inner: pyawaitable.Awaitable, err: Exception) -> int:
        assert str(err) == "test"
        event.set()
        set_err_str(ZeroDivisionError, b"42")
        return -2

    abi.awaitable_await(awaitable, coro(), cb, cb_err)

    with pytest.raises(ZeroDivisionError):
        await awaitable

    assert event.is_set()
    
"""

@limit_leaks("5 KB")
@pytest.mark.asyncio
async def test_await_order():
    data = []

    awaitable = abi.awaitable_new()

    async def echo(value: int) -> int:
        return value
    
    @awaitcallback
    def cb(awaitable_inner: pyawaitable.Awaitable, result: int) -> int:
        data.append(result)
        return 0

    for i in (1, 2, 3):
        abi.awaitable_await(awaitable, echo(i), cb, awaitcallback_err(0))

    await awaitable
    assert data == [1, 2, 3]

@limit_leaks("5 KB")
@pytest.mark.asyncio
async def test_await_cancel():
    data = []

    awaitable = abi.awaitable_new()

    async def echo(value: int) -> int:
        return value
    
    @awaitcallback
    def cb(awaitable_inner: pyawaitable.Awaitable, result: int) -> int:
        abi.awaitable_cancel(awaitable_inner)
        data.append(result)
        return 0

    for i in (1, 2, 3):
        abi.awaitable_await(awaitable, echo(i), cb, awaitcallback_err(0))

    await awaitable
    assert data == [1]

@limit_leaks("5 KB")
@pytest.mark.asyncio
async def test_awaitable_chaining():
    data = []

    awaitable = abi.awaitable_new()

    async def echo(value: int) -> int:
        return value
    
    @awaitcallback
    def cb(awaitable_inner: pyawaitable.Awaitable, result: int) -> int:
        abi.awaitable_cancel(awaitable_inner)
        data.append(result)
        return 0

    awaitable2 = abi.awaitable_new()
    abi.awaitable_await(awaitable2, echo(1), cb, awaitcallback_err(0))
    abi.awaitable_await(awaitable, awaitable2, awaitcallback(0), awaitcallback_err(0))
    
    await awaitable
    assert data == [1]
