# Version Rules

## Capsules

As stated earlier, PyAwaitable loads into extensions via [capsules](https://docs.python.org/3/extending/extending.html#using-capsules). The location of the ABI capsule is `pyawaitable.abi.v<major version>`. As of now, the only ABI is `pyawaitable.abi.v1`.

PyAwaitable's `awaitable.h` provides an `AwaitableABI` structure. This structure is guaranteed to always have the first member be `Py_ssize_t size`. This is the size of the entire structure. Between versions, your header file might have ABI members defined that do not exist on the runtime version. To check this, you 
can use `offsetof`:

```c
if (awaitable_abi->size <= offsetof(awaitable_abi, some_member)
{
    // Use deprecated version of some_member
}
```

## Major Versions (Y.x.x)

Between major versions, there may be any changes made to the ABI. However, the legacy ABI's will still be supported for a period of time. In fact, the new ABI will be opt-in for a while (as in, you would have to define a macro to use the new ABI instead of the legacy one).

## Minior Versions (x.Y.x)

Between minor versions, anything may be added to the ABI, but once something is added, it's signature is frozen. Fields will never change type, move around, or be removed *ever*. If one of these changes occur, it would be in a totally new ABI with the major version bumped.

## Micro Versions (x.x.Y)

Nothing will be added to the ABI between micro versions, and will only add security patches to function implementations. To the user, `x.x.0` will feel the same as `x.x.1` (with the exception of bug fixes).
