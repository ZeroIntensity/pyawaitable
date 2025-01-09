# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased

- Significantly reduced awaitable object size by dynamically allocating it.
- Reduced memory footprint by removing preallocated awaitable objects.
- Objects returned by a PyAwaitable object's `__await__` are now garbage collected (*i.e.*, they don't leak with rare circular references).
- Removed limit on number of stored callbacks or values.
- Switched some user-error messages to `RuntimeError` instead of `SystemError`.
- Implemented a special callback that can defer the calling of a pure python awaitable until when the `PyAwaitable` is awaited.

## [1.3.0] - 2024-10-26

- Added support for `async with` via `pyawaitable_async_with`.

## [1.2.0] - 2024-08-06

- Added getting and setting of value storage.

## [1.1.0] - 2024-08-03

- Changed error message when attempting to await a non-awaitable object (*i.e.*, it has no `__await__`).
- Fixed coroutine iterator reference leak.
- Fixed reference leak in error callbacks.
- Fixed early exit of `pyawaitable_unpack_arb` if a `NULL` value was saved.
- Added integer value saving and unpacking (`pyawaitable_save_int` and `pyawaitable_unpack_int`).
- Callbacks are now preallocated for better performance.
- Fixed reference leak in the coroutine `send()` method.

## [1.0.0] - 2024-06-24

- Initial release.
