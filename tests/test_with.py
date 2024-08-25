from contextlib import asynccontextmanager

import pytest
from conftest import limit_leaks, raising_callback

import pyawaitable
from pyawaitable.bindings import abi, awaitcallback, awaitcallback_err


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


@limit_leaks
@pytest.mark.asyncio
async def test_async_with_exit_error_cb():
    called = False

    @asynccontextmanager
    async def my_context():
        try:
            yield 1
        finally:
            raise ZeroDivisionError

    @awaitcallback_err
    def cb(inner: pyawaitable.PyAwaitable, err: BaseException) -> int:
        assert isinstance(err, ZeroDivisionError)
        nonlocal called
        called = True
        return 0

    aw = abi.new()
    abi.async_with(aw, my_context(), awaitcallback(0), cb)
    await aw
    assert called is True
