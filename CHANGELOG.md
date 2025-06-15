# Changelog

## [2.0.1] - 2025-06-15

-   Pinned dependencies to a specific version to prevent incompatibility problems.

## [2.0.0] - 2025-03-30

-   Moved away from function pointer tables for loading PyAwaitable--everything is now vendored upon installation.
-   Improved performance with compiler optimizations.
-   `PyAwaitable_` prefixes are now required, and the old `pyawaitable_*` functions have been removed.
-   The warning emitted when a PyAwaitable object is not awaited is now a `ResourceWarning` (was a `RuntimeWarning`).
-   `PyAwaitable_AddAwait` now raises a `ValueError` if the passed object is `NULL` or self, and also now raises a `TypeError` if the passed object is not a coroutine.
-   Added a simple CLI, primarily for getting the include directory from `meson-python` (`pyawaitable --include`).
-   PyAwaitable objects now support garbage collection.
-   **Breaking Change:** `PyAwaitable_Init` no longer takes a module object.
-   **Breaking Change:** Renamed `awaitcallback` to `PyAwaitable_Callback`
-   **Breaking Change:** Renamed `awaitcallback_err` to `PyAwaitable_Error`
-   **Breaking Change:** Renamed `defercallback` to `PyAwaitable_Defer`
-   **Breaking Change:** Removed the integer value APIs (`SaveIntValues`, `LoadIntValues`, `SetIntValue`, `GetIntValue`). They proved to be maintenance heavy, unintuitive, and most of all replaceable with the arbitrary values API (via `malloc`ing an integer and storing it).

## [1.4.0] - 2025-02-09

-   Significantly reduced awaitable object size by dynamically allocating it.
-   Reduced memory footprint by removing preallocated awaitable objects.
-   Objects returned by a PyAwaitable object's `__await__` are now garbage collected (_i.e._, they don't leak with rare circular references).
-   Removed limit on number of stored callbacks or values.
-   Switched some user-error messages to `RuntimeError` instead of `SystemError`.
-   Added `PyAwaitable_DeferAwait` for executing code without a coroutine when the awaitable object is called by the event loop.

## [1.3.0] - 2024-10-26

-   Added support for `async with` via `pyawaitable_async_with`.

## [1.2.0] - 2024-08-06

-   Added getting and setting of value storage.

## [1.1.0] - 2024-08-03

-   Changed error message when attempting to await a non-awaitable object (_i.e._, it has no `__await__`).
-   Fixed coroutine iterator reference leak.
-   Fixed reference leak in error callbacks.
-   Fixed early exit of `pyawaitable_unpack_arb` if a `NULL` value was saved.
-   Added integer value saving and unpacking (`pyawaitable_save_int` and `pyawaitable_unpack_int`).
-   Callbacks are now preallocated for better performance.
-   Fixed reference leak in the coroutine `send()` method.

## [1.0.0] - 2024-06-24

-   Initial release.
