#include <Python.h>
#include <pyawaitable.h>

typedef struct {
    PyMemAllocatorEx raw;
    PyMemAllocatorEx mem;
    PyMemAllocatorEx obj;
} AllocHook;

// TODO: Make this thread-safe for concurrent tests
AllocHook hook;

static void *
malloc_fail(void *ctx, size_t size)
{
    return NULL;
}

static void *
calloc_fail(void *ctx, size_t nitems, size_t size)
{
    return NULL;
}

static void *
realloc_fail(void *ctx, void *ptr, size_t newsize)
{
    return NULL;
}

static void
wrapped_free(void *ctx, void *ptr)
{
    PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;
    alloc->free(alloc->ctx, ptr);
}

void
Test_SetNoMemory(void)
{
    PyMemAllocatorEx alloc;
    alloc.malloc = malloc_fail;
    alloc.calloc = calloc_fail;
    alloc.realloc = realloc_fail;
    alloc.free = wrapped_free;

    PyMem_GetAllocator(PYMEM_DOMAIN_RAW, &hook.raw);
    PyMem_GetAllocator(PYMEM_DOMAIN_MEM, &hook.mem);
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &hook.obj);

    alloc.ctx = &hook.raw;
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &alloc);

    alloc.ctx = &hook.raw;
    PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &alloc);

    alloc.ctx = &hook.raw;
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &alloc);
}

void
Test_UnSetNoMemory(void)
{
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &hook.raw);
    PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &hook.mem);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &hook.obj);
}

PyObject *
Test_RunAwaitable(PyObject *awaitable)
{
    PyObject *asyncio = PyImport_ImportModule("asyncio");
    if (asyncio == NULL) {
        return NULL;
    }

    PyObject *loop = PyObject_CallMethod(asyncio, "new_event_loop", "");
    Py_DECREF(asyncio);
    if (loop == NULL) {
        return NULL;
    }

    PyObject *res = PyObject_CallMethod(
        loop,
        "run_until_complete",
        "O",
        awaitable
    );
    // Temporarily remove the error so we can close the loop
    PyObject *err = PyErr_GetRaisedException();
    PyObject *close_res = PyObject_CallMethod(loop, "close", "");
    Py_DECREF(loop);
    PyErr_SetRaisedException(err);

    if (close_res == NULL) {
        // Both failed? Yikes. Let's just show the first one and bail out.
        Py_XDECREF(res);
        return NULL;
    }

    return res;
}

PyObject *
_Test_RunAndCheck(
    PyObject *awaitable,
    PyObject *expected,
    const char *func,
    const char *file,
    int line
)
{
    PyObject *res = Test_RunAwaitable(awaitable);
    if (res == NULL) {
        Py_DECREF(awaitable);
        return NULL;
    }
    Py_DECREF(awaitable);
    if (!Py_Is(res, expected)) {
        PyErr_Format(
            PyExc_AssertionError,
            "test %s at %s:%d expected awaitable to return %R, got %R",
            func,
            file,
            line,
            expected,
            res
        );
        Py_DECREF(res);
        return NULL;
    }
    Py_DECREF(res);
    Py_RETURN_NONE;
}
