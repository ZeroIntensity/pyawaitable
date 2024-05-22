/* *INDENT-OFF* */
#include <Python.h>
#include <pyawaitable/backport.h>
#include <pyawaitable/values.h>
#include <pyawaitable/awaitableobject.h>

int
awaitable_unpack_impl(PyObject *awaitable, ...)
{
    assert(awaitable != NULL);
    AwaitableObject *aw = (AwaitableObject *) awaitable;
    Py_INCREF(awaitable);

    if (aw->aw_values == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "awaitable object has no stored values");
        Py_DECREF(awaitable);
        return -1;
    }

    va_list args;
    va_start(args, awaitable);

    for (int i = 0; i < aw->aw_values_size; i++) {
        PyObject **ptr = va_arg(args, PyObject **);
        if (ptr == NULL) continue;
        *ptr = aw->aw_values[i];
        // borrowed reference
    }

    va_end(args);
    Py_DECREF(awaitable);
    return 0;
}

int
awaitable_save_impl(PyObject *awaitable, Py_ssize_t nargs, ...)
{
    assert(awaitable != NULL);
    assert(nargs != 0);
    Py_INCREF(awaitable);
    AwaitableObject *aw = (AwaitableObject *) awaitable;

    va_list vargs;
    va_start(vargs, nargs);
    Py_ssize_t offset = aw->aw_values_size;

    if (aw->aw_values == NULL)
        aw->aw_values = PyMem_Calloc(
            nargs,
            sizeof(PyObject *)
        );
    else
        aw->aw_values = PyMem_Realloc(
            aw->aw_values,
            sizeof(PyObject *) * (aw->aw_values_size + nargs)
        );

    if (aw->aw_values == NULL) {
        PyErr_NoMemory();
        Py_DECREF(awaitable);
        return -1;
    }

    aw->aw_values_size += nargs;

    for (Py_ssize_t i = offset; i < aw->aw_values_size; i++)
        aw->aw_values[i] = Py_NewRef(va_arg(vargs, PyObject*));

    va_end(vargs);
    Py_DECREF(awaitable);
    return 0;
}

int
awaitable_unpack_arb_impl(PyObject *awaitable, ...)
{
    assert(awaitable != NULL);
    AwaitableObject *aw = (AwaitableObject *) awaitable;
    Py_INCREF(awaitable);

    if (aw->aw_values == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "awaitable object has no stored arbitrary values");
        Py_DECREF(awaitable);
        return -1;
    }

    va_list args;
    va_start(args, awaitable);

    for (int i = 0; i < aw->aw_arb_values_size; i++) {
        void **ptr = va_arg(args, void **);
        if (ptr == NULL) continue;
        *ptr = aw->aw_arb_values[i];
    }

    va_end(args);
    Py_DECREF(awaitable);
    return 0;
}

int
awaitable_save_arb_impl(PyObject *awaitable, Py_ssize_t nargs, ...)
{
    assert(awaitable != NULL);
    assert(nargs != 0);
    Py_INCREF(awaitable);
    AwaitableObject *aw = (AwaitableObject *) awaitable;

    va_list vargs;
    va_start(vargs, nargs);
    Py_ssize_t offset = aw->aw_arb_values_size;

    if (aw->aw_arb_values == NULL)
        aw->aw_arb_values = PyMem_Calloc(
            nargs,
            sizeof(void *)
        );
    else
        aw->aw_arb_values = PyMem_Realloc(
            aw->aw_arb_values,
            sizeof(void *) * (aw->aw_arb_values_size + nargs)
        );

    if (aw->aw_arb_values == NULL) {
        PyErr_NoMemory();
        Py_DECREF(awaitable);
        return -1;
    }

    aw->aw_arb_values_size += nargs;

    for (Py_ssize_t i = offset; i < aw->aw_arb_values_size; i++)
        aw->aw_arb_values[i] = va_arg(vargs, void *);

    va_end(vargs);
    Py_DECREF(awaitable);
    return 0;
}
