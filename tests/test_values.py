import asyncio
import ctypes

import pytest
from conftest import limit_leaks

import pyawaitable
from pyawaitable.bindings import (abi, add_await, awaitcallback,
                                  awaitcallback_err)


@limit_leaks
@pytest.mark.asyncio
async def test_store_values():
    awaitable = abi.new()

    async def echo(value: int) -> int:
        await asyncio.sleep(0)
        return value

    data = ctypes.py_object([1, 2, 3])
    some_val = ctypes.py_object("test")

    abi.save(awaitable, 2, data, some_val)

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: int) -> int:
        data_inner = ctypes.py_object()
        some_val_inner = ctypes.py_object()
        abi.unpack(
            awaitable_inner,
            ctypes.byref(data_inner),
            ctypes.byref(some_val_inner),
        )
        assert data.value == data_inner.value
        assert some_val.value == some_val_inner.value
        assert abi.get(awaitable, 0) == data.value
        data.value.append(4)

        with pytest.raises(IndexError):
            abi.get(awaitable, 2)

        with pytest.raises(IndexError):
            abi.get(awaitable, 200)

        with pytest.raises(IndexError):
            abi.get(awaitable, -2)

        with pytest.raises(IndexError):
            abi.set(awaitable, 2, "test")

        with pytest.raises(IndexError):
            abi.set(awaitable, -42, "test")

        abi.set(awaitable, 1, "hello")
        assert abi.get(awaitable, 1) == "hello"
        foo = ctypes.py_object("foo")
        bar = ctypes.py_object("bar")

        abi.save(awaitable, 2, foo, bar)

        foo_inner = ctypes.py_object()
        bar_inner = ctypes.py_object()

        abi.unpack(
            awaitable,
            None,
            None,
            ctypes.byref(foo_inner),
            ctypes.byref(bar_inner),
        )
        assert foo_inner.value == "foo"
        assert bar_inner.value == "bar"

        assert abi.get(awaitable, 2) == "foo"
        assert abi.get(awaitable, 3) == "bar"

        return 0

    add_await(awaitable, echo(42), cb, awaitcallback_err(0))
    await awaitable
    assert data.value == [1, 2, 3, 4]


@limit_leaks
@pytest.mark.asyncio
async def test_store_arb_values():
    awaitable = abi.new()

    async def echo(value: int) -> int:
        await asyncio.sleep(0)
        return value

    buffer = ctypes.create_string_buffer(b"test")
    abi.save_arb(awaitable, 1, ctypes.byref(buffer))

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: int) -> int:
        buffer_inner = ctypes.c_char_p()
        abi.unpack_arb(awaitable_inner, ctypes.byref(buffer_inner))
        assert buffer_inner.value == b"test"
        assert (
            ctypes.cast(abi.get_arb(awaitable, 0), ctypes.c_char_p).value
            == b"test"
        )
        unicode = ctypes.create_unicode_buffer("hello")

        abi.save_arb(
            awaitable,
            1,
            ctypes.byref(unicode),
        )
        unicode_inner = ctypes.c_wchar_p()
        abi.unpack_arb(
            awaitable_inner,
            None,
            ctypes.byref(unicode_inner),
        )
        assert unicode_inner.value == "hello"

        assert (
            ctypes.cast(
                abi.get_arb(awaitable_inner, 1),
                ctypes.c_wchar_p,
            ).value
            == "hello"
        )

        with pytest.raises(IndexError):
            abi.get_arb(awaitable_inner, 2)

        with pytest.raises(IndexError):
            abi.get_arb(awaitable_inner, 300)

        with pytest.raises(IndexError):
            abi.get_arb(awaitable_inner, -10)

        assert (
            ctypes.cast(abi.get_arb(awaitable_inner, 0), ctypes.c_char_p).value
            == b"test"
        )

        with pytest.raises(IndexError):
            abi.set_arb(awaitable_inner, 10, None)

        abi.set_arb(awaitable_inner, 0, ctypes.c_char_p(b"hello"))
        assert (
            ctypes.cast(abi.get_arb(awaitable_inner, 0), ctypes.c_char_p).value
            == b"hello"
        )
        return 0

    add_await(awaitable, echo(42), cb, awaitcallback_err(0))
    await awaitable


@limit_leaks
@pytest.mark.asyncio
async def test_null_save_arb():
    awaitable = abi.new()

    async def echo(value: int) -> int:
        await asyncio.sleep(0)
        return value

    buffer = ctypes.create_string_buffer(b"test")
    buffer2 = ctypes.create_string_buffer(b"hello")
    abi.save_arb(awaitable, 3, ctypes.byref(buffer), None, buffer2)

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: int) -> int:
        buffer_inner = ctypes.c_char_p()
        null = ctypes.c_void_p()
        buffer2_inner = ctypes.c_char_p()
        abi.unpack_arb(
            awaitable_inner,
            ctypes.byref(buffer_inner),
            ctypes.byref(null),
            ctypes.byref(buffer2_inner),
        )
        assert buffer_inner.value == b"test"
        assert buffer2_inner.value == b"hello"
        return 0

    add_await(awaitable, echo(42), cb, awaitcallback_err(0))
    await awaitable


@limit_leaks
@pytest.mark.asyncio
async def test_int_values():
    awaitable = abi.new()

    abi.save_int(
        awaitable,
        3,
        ctypes.c_long(42),
        ctypes.c_long(3000),
        ctypes.c_long(-10),
    )

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: int) -> int:
        first = ctypes.c_long()
        second = ctypes.c_long()
        third = ctypes.c_long()
        abi.unpack_int(
            awaitable_inner,
            ctypes.byref(first),
            ctypes.byref(second),
            ctypes.byref(third),
        )
        assert first.value == 42
        assert second.value == 3000
        assert third.value == -10
        assert abi.get_int(awaitable_inner, 0) == 42

        with pytest.raises(IndexError):
            abi.set_int(awaitable_inner, 10, ctypes.c_long(4))

        with pytest.raises(IndexError):
            abi.set_int(awaitable_inner, 3, ctypes.c_long(4))

        abi.set_int(awaitable_inner, 2, ctypes.c_long(4))
        abi.unpack_int(
            awaitable_inner,
            ctypes.byref(first),
            ctypes.byref(second),
            ctypes.byref(third),
        )

        assert first.value == 42
        assert second.value == 3000
        assert third.value == 4

        abi.set_int(awaitable_inner, 0, ctypes.c_long(-400))
        assert abi.get_int(awaitable_inner, 0) == -400
        assert abi.get_int(awaitable_inner, 2) == 4

        with pytest.raises(IndexError):
            abi.get_int(awaitable_inner, 3)

        with pytest.raises(IndexError):
            abi.get_int(awaitable_inner, 100)

        with pytest.raises(IndexError):
            abi.get_int(awaitable_inner, -10)

        abi.save_int(
            awaitable,
            3,
            ctypes.c_long(1),
            ctypes.c_long(2),
            ctypes.c_long(3),
        )

        assert abi.get_int(awaitable_inner, 5) == 3
        assert abi.get_int(awaitable_inner, 3) == 1
        abi.set_int(awaitable_inner, 5, ctypes.c_long(1000))
        assert abi.get_int(awaitable_inner, 5) == 1000

        with pytest.raises(IndexError):
            abi.get_int(awaitable_inner, 10)

        return 0

    async def coro(): ...

    add_await(awaitable, coro(), cb, awaitcallback_err(0))
    await awaitable
