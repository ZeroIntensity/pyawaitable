import asyncio
import ctypes
from collections.abc import Coroutine
from contextlib import asynccontextmanager

import _pyawaitable_test
import pytest
from conftest import limit_leaks

import pyawaitable
from pyawaitable.bindings import (abi, add_await, awaitcallback,
                                  awaitcallback_err)

raising_callback = ctypes.cast(
    _pyawaitable_test.raising_callback, awaitcallback
)
raising_err_callback = ctypes.cast(
    _pyawaitable_test.raising_err_callback, awaitcallback_err
)


@limit_leaks
@pytest.mark.asyncio
async def test_new():
    awaitable = abi.new()
    assert isinstance(awaitable, pyawaitable.PyAwaitable)
    assert isinstance(awaitable, Coroutine)
    await awaitable
    await asyncio.create_task(abi.new())
    await abi.new()


@limit_leaks
@pytest.mark.asyncio
async def test_await():
    called = False

    async def coro():
        await asyncio.sleep(0)
        nonlocal called
        called = True

    awaitable = abi.new()
    add_await(awaitable, coro(), awaitcallback(0), awaitcallback_err(0))
    await awaitable
    assert called is True


@limit_leaks
@pytest.mark.asyncio
async def test_await_cb():
    awaitable = abi.new()

    async def coro(value: int):
        await asyncio.sleep(0)
        return value * 2

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: int) -> int:
        assert awaitable_inner is awaitable
        assert result == 42
        return 0

    add_await(awaitable, coro(21), cb, awaitcallback_err(0))
    await awaitable


@limit_leaks
@pytest.mark.asyncio
async def test_await_cb_err():
    awaitable = abi.new()

    async def coro_raise() -> float:
        await asyncio.sleep(0)
        return 0 / 0

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: float) -> int:
        raise RuntimeError(b"no good!")

    @awaitcallback_err
    def cb_err(
        awaitable_inner: pyawaitable.PyAwaitable,
        err: Exception,
    ) -> int:
        assert isinstance(err, ZeroDivisionError)
        return 0

    add_await(awaitable, coro_raise(), cb, cb_err)
    await awaitable


@limit_leaks
@pytest.mark.asyncio
async def test_await_cb_err_cb():
    awaitable = abi.new()

    async def coro() -> int:
        await asyncio.sleep(0)
        return 42

    @awaitcallback_err
    def cb_err(
        awaitable_inner: pyawaitable.PyAwaitable,
        err: Exception,
    ) -> int:
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


@limit_leaks
@pytest.mark.asyncio
async def test_await_cb_noerr():
    awaitable = abi.new()

    async def coro() -> int:
        await asyncio.sleep(0)
        return 42

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: int) -> int:
        return -1

    add_await(awaitable, coro(), cb, awaitcallback_err(0))

    with pytest.raises(SystemError):
        await awaitable


@limit_leaks
@pytest.mark.asyncio
async def test_await_cb_err_restore():
    awaitable = abi.new()
    called = False

    async def coro() -> int:
        await asyncio.sleep(0)
        return 42

    @awaitcallback_err
    def cb_err(
        awaitable_inner: pyawaitable.PyAwaitable,
        err: Exception,
    ) -> int:
        assert str(err) == "test"
        nonlocal called
        called = True
        return -1

    add_await(awaitable, coro(), raising_callback, cb_err)

    with pytest.raises(RuntimeError):
        await awaitable

    assert called is True


@limit_leaks
@pytest.mark.asyncio
async def test_await_cb_err_norestore():
    awaitable = abi.new()
    called = False

    async def coro() -> int:
        nonlocal called
        called = True
        await asyncio.sleep(0)
        return 42

    add_await(awaitable, coro(), raising_callback, raising_err_callback)

    with pytest.raises(ZeroDivisionError):
        await awaitable

    assert called is True


@limit_leaks
@pytest.mark.asyncio
async def test_await_order():
    data = []

    awaitable = abi.new()

    async def echo(value: int) -> int:
        await asyncio.sleep(0)
        return value

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: int) -> int:
        data.append(result)
        return 0

    for i in (1, 2, 3):
        add_await(awaitable, echo(i), cb, awaitcallback_err(0))

    await awaitable
    assert data == [1, 2, 3]


@limit_leaks
@pytest.mark.asyncio
@pytest.mark.filterwarnings(
    "ignore::RuntimeWarning"
)  # Second and third iteration of echo() are skipped, resulting in a warning
async def test_await_cancel():
    data = []

    awaitable = abi.new()

    async def echo(value: int) -> int:
        await asyncio.sleep(0)
        return value

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: int) -> int:
        assert result == 1
        abi.cancel(awaitable_inner)
        data.append(result)
        return 0

    for i in (1, 2, 3):
        add_await(awaitable, echo(i), cb, awaitcallback_err(0))

    await awaitable
    assert data == [1]


@limit_leaks
@pytest.mark.asyncio
async def test_awaitable_chaining():
    data = []

    awaitable = abi.new()

    async def echo(value: int) -> int:
        await asyncio.sleep(0)
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


@limit_leaks
@pytest.mark.asyncio
async def test_coro_raise():
    awaitable = abi.new()

    async def coro() -> None:
        await asyncio.sleep(0)
        raise ZeroDivisionError("test")

    add_await(awaitable, coro(), awaitcallback(0), awaitcallback_err(0))

    with pytest.raises(ZeroDivisionError):
        await awaitable


@limit_leaks
@pytest.mark.asyncio
async def test_await_no_cb_raise():
    awaitable = abi.new()

    add_await(awaitable, 42, awaitcallback(0), awaitcallback_err(0))

    with pytest.raises(TypeError):
        await awaitable


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
async def test_c_built_extension():
    async def hello():
        await asyncio.sleep(0)
        return 42

    assert (await _pyawaitable_test.test(hello())) == 42
    assert (await _pyawaitable_test.test2()) is None


@limit_leaks
@pytest.mark.asyncio
async def test_set_results():
    awaitable = abi.new()

    async def coro():
        await asyncio.sleep(0)
        return "abc"

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: str):
        abi.set_result(awaitable_inner, 42)
        return 0

    add_await(awaitable, coro(), cb, awaitcallback(0))
    assert (await awaitable) == 42


@limit_leaks
@pytest.mark.asyncio
async def test_await_function():
    awaitable = abi.new()
    called: bool = False

    async def coro(value: int, suffix: str) -> str:
        await asyncio.sleep(0)
        return str(value * 2) + suffix

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: str):
        nonlocal called
        called = True
        assert result == "42hello"
        return 0

    abi.await_function(
        awaitable, coro, b"is", cb, awaitcallback_err(0), 21, b"hello"
    )
    await awaitable
    assert called is True


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


@limit_leaks
@pytest.mark.asyncio
async def test_async_with():
    called = False

    @asynccontextmanager
    async def my_context():
        try:
            yield 1
        finally:
            nonlocal called
            called = True

    @awaitcallback
    def inner(inner: pyawaitable.PyAwaitable, val: int) -> int:
        assert val == 1
        return 0

    aw = abi.new()
    abi.async_with(aw, my_context(), inner, awaitcallback_err(0))

    class Half:
        def __aenter__(self): ...

    class OtherHalf:
        def __aexit__(self, *args): ...

    with pytest.raises(TypeError):
        abi.async_with(aw, 1, inner, awaitcallback_err(0))

    with pytest.raises(TypeError):
        abi.async_with(aw, Half(), inner, awaitcallback_err(0))

    with pytest.raises(TypeError):
        abi.async_with(aw, OtherHalf(), inner, awaitcallback_err(0))

    await aw
    assert called is True


@limit_leaks
@pytest.mark.asyncio
async def test_async_with_no_callback():
    called = False

    @asynccontextmanager
    async def my_context():
        try:
            yield 1
        finally:
            nonlocal called
            called = True

    aw = abi.new()
    abi.async_with(aw, my_context(), awaitcallback(0), awaitcallback_err(0))
    await aw
    assert called is True


@limit_leaks
@pytest.mark.asyncio
async def test_async_with_no_callback_error():
    @asynccontextmanager
    async def my_context():
        yield 1
        raise ZeroDivisionError

    aw = abi.new()
    abi.async_with(aw, my_context(), awaitcallback(0), awaitcallback_err(0))

    with pytest.raises(ZeroDivisionError):
        await aw


@limit_leaks
@pytest.mark.asyncio
async def test_async_with_no_callback_exit_error():
    @asynccontextmanager
    async def my_context():
        try:
            yield 1
        finally:
            raise ZeroDivisionError

    aw = abi.new()
    abi.async_with(aw, my_context(), awaitcallback(0), awaitcallback_err(0))

    with pytest.raises(ZeroDivisionError):
        await aw


@limit_leaks
@pytest.mark.asyncio
async def test_async_with_cb_error():
    called = False

    @asynccontextmanager
    async def my_context():
        try:
            yield 1
        finally:
            nonlocal called
            called = True

    aw = abi.new()
    abi.async_with(aw, my_context(), raising_callback, awaitcallback_err(0))
    await aw
    assert called is True


@limit_leaks
@pytest.mark.asyncio
async def test_async_with_exit_error():
    @asynccontextmanager
    async def my_context():
        try:
            yield 1
        finally:
            raise ZeroDivisionError

    called = False

    @awaitcallback_err
    def cb(inner: pyawaitable.PyAwaitable, err: BaseException) -> int:
        nonlocal called
        called = True
        assert isinstance(err, ZeroDivisionError)
        return 0

    aw = abi.new()
    abi.async_with(aw, my_context(), awaitcallback(0), cb)
    await aw
    assert called is True


@limit_leaks
@pytest.mark.asyncio
async def test_async_with_enter_error():
    called = False

    @asynccontextmanager
    async def my_context():
        try:
            yield 1
            raise ZeroDivisionError
        finally:
            nonlocal called
            called = True

    aw = abi.new()
    abi.async_with(aw, my_context(), awaitcallback(0), awaitcallback_err(0))
    with pytest.raises(ZeroDivisionError):
        await aw
    assert called is True
