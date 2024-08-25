# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased

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
