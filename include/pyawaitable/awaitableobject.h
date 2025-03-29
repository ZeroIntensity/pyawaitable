#ifndef PYAWAITABLE_AWAITABLE_H
#define PYAWAITABLE_AWAITABLE_H

#include <Python.h>
#include <stdbool.h>

#include <pyawaitable/array.h>
#include <pyawaitable/dist.h>

typedef int (*PyAwaitable_Callback)(PyObject *, PyObject *);
typedef int (*PyAwaitable_Error)(PyObject *, PyObject *);
typedef int (*PyAwaitable_Defer)(PyObject *);

typedef struct _pyawaitable_callback {
    PyObject *coro;
    PyAwaitable_Callback callback;
    PyAwaitable_Error err_callback;
    bool done;
} _PyAwaitable_MANGLE(pyawaitable_callback);

struct _PyAwaitableObject {
    PyObject_HEAD

    pyawaitable_array aw_callbacks;
    pyawaitable_array aw_object_values;
    pyawaitable_array aw_arbitrary_values;

    /* Index of current callback */
    Py_ssize_t aw_state;
    /* Is the awaitable done? */
    bool aw_done;
    /* Was the awaitable awaited? */
    bool aw_awaited;
    /* Strong reference to the result of the coroutine. */
    PyObject *aw_result;
    /* Strong reference to the genwrapper. */
    PyObject *aw_gen;
    /* Set to 1 if the object was cancelled, for introspection against callbacks */
    int aw_recently_cancelled;
};

typedef struct _PyAwaitableObject PyAwaitableObject;
_PyAwaitable_INTERNAL_DATA(PyTypeObject) PyAwaitable_Type;

_PyAwaitable_API(int)
PyAwaitable_SetResult(PyObject * awaitable, PyObject * result);

_PyAwaitable_API(int)
PyAwaitable_AddAwait(
    PyObject * aw,
    PyObject * coro,
    PyAwaitable_Callback cb,
    PyAwaitable_Error err
);

_PyAwaitable_API(int)
PyAwaitable_DeferAwait(PyObject * aw, PyAwaitable_Defer cb);

_PyAwaitable_API(void)
PyAwaitable_Cancel(PyObject * aw);

_PyAwaitable_INTERNAL(PyObject *)
awaitable_next(PyObject * self);

_PyAwaitable_API(PyObject *)
PyAwaitable_New(void);

#endif
