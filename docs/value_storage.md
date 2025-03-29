# Managing State Between Callbacks

So far, all of our examples haven't needed any transfer of state between the `PyAwaitable_AddAwait` callsite and the time the coroutine is executed (_i.e._, in a callback). You might have noticed that the callbacks don't take a `void *arg` parameter to make up for C's lack of closures, so how do we manage state?

First, let's get an example of what we might want state for. Our goal is to implement a function that takes a paremeter and does something wiith it against the result of a coroutine. For example:

```py
async def multiply_async(
    number: int,
    get_number_io: Callable[[], collections.abc.Awaitable[int]]
) -> str:
    value = await get_number_io()
    return value * number
```

## Introducing Value Storage

Instead of `void *arg` parameters, PyAwaitable provides APIs for storing state directly on the PyAwaitable object. There are two types of value storage:

-   Object value storage; `PyObject *` pointers that PyAwaitable correctly stores references to.
-   Arbitrary value storage; `void *` pointers that PyAwaitable never dereferences--it's your job to manage it.

Value storage is generally a lot more convenient than something like a `void *arg`, because you don't have to define any `struct` or make an extra allocations. It's especially convenient in the `PyObject *` case, because you don't have to worry about dealing with their reference counts or traversing reference cycles. And, even if a single state `struct` is more convenient for your case, it's easy to implement it with the arbitrary values API.

There are four parts to the value APIs, each with a variant for object and arbitrary values:

-   Saving values; `PyAwaitable_SaveValues`/`PyAwaitable_SaveArbValues`.
-   Unpacking values; `PyAwaitable_UnpackValues`/`PyAwaitable_UnpackArbValues`.
-   Getting values; `PyAwaitable_GetValue`/`PyAwaitable_GetArbValue`.
-   Setting values; `PyAwaitable_SetValue`/`PyAwaitable_SetArbValue`.
