import pyawaitable
import ctypes
from ctypes import pythonapi
import pytest
import asyncio
import platform
from typing import Callable, Any
from typing_extensions import Self
import _pyawaitable_test
from collections.abc import Coroutine

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
        abi = ctypes.cast(get_pointer(capsule, capsule_name), ctypes.POINTER(cls))

        return abi.contents

    def __init_subclass__(cls, **kwargs) -> None:
        super().__init_subclass__(**kwargs)
        # Assume that _fields_ is a list
        cls._fields_.insert(0, ("size", ctypes.c_ssize_t))  # type: ignore

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
        ("new", ctypes.PYFUNCTYPE(ctypes.py_object)),
        (
            "await",
            ctypes.PYFUNCTYPE(
                ctypes.c_int,
                ctypes.py_object,
                ctypes.py_object,
                awaitcallback,
                awaitcallback_err,
            ),
        ),
        ("cancel", ctypes.PYFUNCTYPE(None, ctypes.py_object)),
        (
            "set_result",
            ctypes.PYFUNCTYPE(ctypes.c_int, ctypes.py_object, ctypes.py_object),
        ),
        ("save", ctypes.PYFUNCTYPE(ctypes.c_int, ctypes.py_object, ctypes.c_ssize_t)),
        (
            "save_arb",
            ctypes.PYFUNCTYPE(ctypes.c_int, ctypes.py_object, ctypes.c_ssize_t),
        ),
        ("unpack", ctypes.PYFUNCTYPE(ctypes.c_int, ctypes.py_object)),
        ("unpack_arb", ctypes.PYFUNCTYPE(ctypes.c_int, ctypes.py_object)),
    ]


abi = AwaitableABI.from_capsule(pyawaitable.abi.v1)
add_await = getattr(abi, "await")

LEAK_LIMIT: str = "10 KB"


def limit_leaks(memstring: str):
    def decorator(func: Callable):
        if platform.system() != "Windows":
            func = pytest.mark.limit_leaks(memstring)(func)
            return func
        else:
            return func

    return decorator


@limit_leaks(LEAK_LIMIT)
@pytest.mark.asyncio
async def test_new():
    awaitable = abi.new()
    assert isinstance(awaitable, pyawaitable.PyAwaitable)
    assert isinstance(awaitable, Coroutine)
    await awaitable
    await asyncio.create_task(abi.new())
    await abi.new()


@pytest.mark.limit_leaks(LEAK_LIMIT)
@pytest.mark.asyncio
async def test_object_cleanup():
    for i in range(100):
        await abi.new()


@limit_leaks(LEAK_LIMIT)
@pytest.mark.asyncio
async def test_await():
    event = asyncio.Event()

    async def coro():
        event.set()

    awaitable = abi.new()
    add_await(awaitable, coro(), awaitcallback(0), awaitcallback_err(0))
    await awaitable
    assert event.is_set()


@limit_leaks(LEAK_LIMIT)
@pytest.mark.asyncio
async def test_await_cb():
    awaitable = abi.new()

    async def coro(value: int):
        return value * 2

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: int) -> int:
        assert awaitable_inner is awaitable
        assert result == 42
        return 0

    add_await(awaitable, coro(21), cb, awaitcallback_err(0))
    await awaitable


@limit_leaks(LEAK_LIMIT)
@pytest.mark.asyncio
async def test_await_cb_err():
    awaitable = abi.new()

    async def coro_raise() -> float:
        return 0 / 0

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: float) -> int:
        set_err_str(RuntimeError, b"no good!")
        return -1

    @awaitcallback_err
    def cb_err(awaitable_inner: pyawaitable.PyAwaitable, err: Exception) -> int:
        assert isinstance(err, ZeroDivisionError)
        return 0

    add_await(awaitable, coro_raise(), cb, cb_err)
    await awaitable


raising_callback = ctypes.cast(_pyawaitable_test.raising_callback, awaitcallback)

"""
@limit_leaks(LEAK_LIMIT)
@pytest.mark.asyncio
async def test_await_cb_err_cb():
    awaitable = abi.new()

    async def coro() -> int:
        return 42

    @awaitcallback_err
    def cb_err(awaitable_inner: pyawaitable.PyAwaitable, err: Exception) -> int:
        assert isinstance(err, RuntimeError)
        assert str(err) == "test"
        return 0

    add_await(
        awaitable,
        coro(),
        raising_callback,
        cb_err,
    )
    await awaitable
"""


@limit_leaks(LEAK_LIMIT)
@pytest.mark.asyncio
async def test_await_cb_noerr():
    awaitable = abi.new()

    async def coro() -> int:
        return 42

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: int) -> int:
        return -1

    add_await(awaitable, coro(), cb, awaitcallback_err(0))

    with pytest.raises(SystemError):
        await awaitable


"""

@limit_leaks(LEAK_LIMIT)
@pytest.mark.asyncio
async def test_await_cb_err_restore():
    awaitable = abi.new()
    event = asyncio.Event()

    async def coro() -> int:
        return 42
    
    @awaitcallback_err
    def cb_err(awaitable_inner: pyawaitable.PyAwaitable, err: Exception) -> int:
        assert str(err) == "test"
        event.set()
        return -1

    add_await(awaitable, coro(), raising_callback, cb_err)

    with pytest.raises(RuntimeError):
        await awaitable

    assert event.is_set()

@limit_leaks(LEAK_LIMIT)
@pytest.mark.asyncio
async def test_await_cb_err_norestore():
    awaitable = abi.new()
    event = asyncio.Event()

    async def coro() -> int:
        return 42

    @awaitcallback_err
    def cb_err(awaitable_inner: pyawaitable.PyAwaitable, err: Exception) -> int:
        assert str(err) == "test"
        event.set()
        set_err_str(ZeroDivisionError, b"42")
        return -2

    add_await(awaitable, coro(), raising_callback, cb_err)

    with pytest.raises(ZeroDivisionError):
        await awaitable

    assert event.is_set()
"""


@limit_leaks(LEAK_LIMIT)
@pytest.mark.asyncio
async def test_await_order():
    data = []

    awaitable = abi.new()

    async def echo(value: int) -> int:
        return value

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: int) -> int:
        data.append(result)
        return 0

    for i in (1, 2, 3):
        add_await(awaitable, echo(i), cb, awaitcallback_err(0))

    await awaitable
    assert data == [1, 2, 3]


@limit_leaks(LEAK_LIMIT)
@pytest.mark.asyncio
async def test_await_cancel():
    data = []

    awaitable = abi.new()

    async def echo(value: int) -> int:
        return value

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: int) -> int:
        abi.cancel(awaitable_inner)
        data.append(result)
        return 0

    for i in (1, 2, 3):
        add_await(awaitable, echo(i), cb, awaitcallback_err(0))

    await awaitable
    assert data == [1]


@limit_leaks(LEAK_LIMIT)
@pytest.mark.asyncio
async def test_awaitable_chaining():
    data = []

    awaitable = abi.new()

    async def echo(value: int) -> int:
        return value

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: int) -> int:
        abi.cancel(awaitable_inner)
        data.append(result)
        return 0

    awaitable2 = abi.new()
    add_await(awaitable2, echo(1), cb, awaitcallback_err(0))
    add_await(awaitable, awaitable2, awaitcallback(0), awaitcallback_err(0))

    await awaitable
    assert data == [1]


@limit_leaks(LEAK_LIMIT)
@pytest.mark.asyncio
async def test_coro_raise():
    awaitable = abi.new()

    async def coro() -> None:
        raise ZeroDivisionError("test")

    add_await(awaitable, coro(), awaitcallback(0), awaitcallback_err(0))

    with pytest.raises(ZeroDivisionError):
        await awaitable


@limit_leaks(LEAK_LIMIT)
@pytest.mark.asyncio
async def test_await_no_cb_raise():
    awaitable = abi.new()

    add_await(awaitable, 42, awaitcallback(0), awaitcallback_err(0))

    with pytest.raises(TypeError):
        await awaitable


@limit_leaks(LEAK_LIMIT)
@pytest.mark.asyncio
async def test_store_values():
    awaitable = abi.new()

    async def echo(value: int) -> int:
        return value

    data = ctypes.py_object([1, 2, 3])
    some_val = ctypes.py_object("test")

    abi.save(awaitable, 2, data, some_val)

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: int) -> int:
        data_inner = ctypes.py_object()
        some_val_inner = ctypes.py_object()
        abi.unpack(
            awaitable_inner, ctypes.byref(data_inner), ctypes.byref(some_val_inner)
        )
        assert data.value == data_inner.value
        assert some_val.value == some_val_inner.value
        data.value.append(4)
        return 0

    add_await(awaitable, echo(42), cb, awaitcallback_err(0))
    await awaitable
    assert data.value == [1, 2, 3, 4]


@limit_leaks(LEAK_LIMIT)
@pytest.mark.asyncio
async def test_store_arb_values():
    awaitable = abi.new()

    async def echo(value: int) -> int:
        return value

    buffer = ctypes.create_string_buffer(b"test")
    abi.save_arb(awaitable, 1, ctypes.byref(buffer))

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: int) -> int:
        buffer_inner = ctypes.c_char_p()
        abi.unpack_arb(awaitable_inner, ctypes.byref(buffer_inner))
        assert buffer_inner.value == b"test"
        return 0

    add_await(awaitable, echo(42), cb, awaitcallback_err(0))
    await awaitable


@pytest.mark.asyncio
@limit_leaks(LEAK_LIMIT)
async def test_c_built_extension():
    async def hello():
        await asyncio.sleep(0)
        return 42

    assert (await _pyawaitable_test.test(hello())) == 42
    assert (await _pyawaitable_test.test2()) is None


@pytest.mark.asyncio
@limit_leaks(LEAK_LIMIT)
async def test_set_results():
    awaitable = abi.new()

    async def coro():
        return "abc"

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: str):
        abi.set_result(awaitable_inner, 42)
        return 0

    add_await(awaitable, coro(), cb, awaitcallback(0))
    assert (await awaitable) == 42
