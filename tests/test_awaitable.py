import asyncio
from collections.abc import Coroutine

import pytest
from conftest import limit_leaks

import pyawaitable
from pyawaitable.bindings import (
    abi,
    add_await,
    awaitcallback,
    awaitcallback_err,
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
        for _ in range(10000):
            await asyncio.sleep(0)
        nonlocal called
        called = True

    awaitable = abi.new()
    add_await(awaitable, coro(), awaitcallback(0), awaitcallback_err(0))
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
async def test_set_results_tracked_type():
    awaitable = abi.new()

    class TestObject:
        def __init__(self) -> None:
            self.value = [1, 2, 3]

    async def coro():
        pass

    @awaitcallback
    def cb(awaitable_inner: pyawaitable.PyAwaitable, result: str):
        obj = TestObject()
        obj.value.append(4)
        abi.set_result(awaitable_inner, obj)
        return 0

    add_await(awaitable, coro(), cb, awaitcallback(0))
    value = await awaitable
    assert isinstance(value, TestObject)
    assert value.value == [1, 2, 3, 4]


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
async def test_c_built_extension():
    async def hello():
        await asyncio.sleep(0)
        return 42

    assert (await _pyawaitable_test.test(hello())) == 42
    assert (await _pyawaitable_test.test2()) is None
