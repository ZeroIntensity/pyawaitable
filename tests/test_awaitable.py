import asyncio
from collections.abc import Coroutine

import pytest
from conftest import limit_leaks, raising_callback, raising_err_callback

import pyawaitable
from pyawaitable.bindings import (abi, add_await, awaitcallback,
                                  awaitcallback_err)


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
