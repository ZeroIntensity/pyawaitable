import asyncio

import pytest
from conftest import limit_leaks, raising_callback, raising_err_callback

import pyawaitable
from pyawaitable.bindings import (abi, add_await, awaitcallback,
                                  awaitcallback_err, defer_callback,
                                  defer_await)


@limit_leaks
@pytest.mark.asyncio
async def test_await_callback_gets_value():
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
async def test_error_callback_used_instead():
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
async def test_callback_exception_given_to_error_callback():
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
async def test_callback_error_propagates():
    awaitable = abi.new()

    async def coro() -> int:
        await asyncio.sleep(0)
        return 42

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: int) -> int:
        return -1

    add_await(awaitable, coro(), cb, awaitcallback_err(0))

    with pytest.raises(RuntimeError):
        await awaitable


@limit_leaks
@pytest.mark.asyncio
async def test_error_callback_gets_exception():
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
async def test_error_callback_with_error_propagates():
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
async def test_a_lot_of_coroutines():
    awaitable = abi.new()
    amount = 500

    awaited = 0
    called = 0

    async def coro():
        await asyncio.sleep(0)
        nonlocal awaited
        awaited += 1

    @awaitcallback
    def callback(awaitable: pyawaitable.PyAwaitable, result: None) -> int:
        assert result is None
        nonlocal called
        called += 1
        return 0
    
    for _ in range(amount):
        add_await(awaitable, coro(), callback, awaitcallback_err(0))

    await awaitable
    assert called == amount
    assert awaited == amount

@limit_leaks
@pytest.mark.asyncio
async def test_deferred_await():
    called = 0
    awaitable = abi.new()

    async def coro(value: int):
        await asyncio.sleep(0)
        return value * 2

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: int) -> int:
        assert awaitable_inner is awaitable
        assert result == 42
        nonlocal called
        called += 1
        return 0

    @defer_callback
    def defer_cb(awaitable: pyawaitable.PyAwaitable):
        nonlocal called
        called += 1
        add_await(awaitable, coro(21), cb, awaitcallback_err(0))
        return 0

    defer_await(awaitable, defer_cb)
    await awaitable
    assert called == 2
