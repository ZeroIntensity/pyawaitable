import asyncio

import pytest
from conftest import limit_leaks, raising_callback, raising_err_callback

import pyawaitable
from pyawaitable.bindings import (abi, add_await, awaitcallback,
                                  awaitcallback_err)


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
