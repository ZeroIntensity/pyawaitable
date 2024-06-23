#include <Python.h>
#include <pyawaitable/backport.h>
#include <pyawaitable/values.h>
#include <pyawaitable/awaitableobject.h>
#define UNPACK(arr, tp, err)                                     \
        assert(awaitable != NULL);                               \
        PyAwaitableObject *aw = (PyAwaitableObject *) awaitable; \
        Py_INCREF(awaitable);                                    \
        if (arr[0] == NULL) {                                    \
            PyErr_SetString(                                     \
    PyExc_ValueError,                                            \
    "pyawaitable: awaitable object has no stored " err           \
            );                                                   \
            Py_DECREF(awaitable);                                \
            return -1;                                           \
        }                                                        \
        va_list args;                                            \
        va_start(args, awaitable);                               \
        for (int i = 0; i < VALUE_ARRAY_SIZE; ++i) {             \
            if (!arr[i])                                         \
            break;                                               \
            tp ptr = va_arg(args, tp);                           \
            if (ptr == NULL)                                     \
            continue;                                            \
            *ptr = arr[i];                                       \
        }                                                        \
        va_end(args);                                            \
        Py_DECREF(awaitable);                                    \
        return 0

#define SAVE_ERR(err)                                     \
        "pyawaitable: " err " array has a capacity of 32" \
        ", so storing %ld more would overflow it"         \

#define SAVE(arr, index, tp, err)                                \
        assert(awaitable != NULL);                               \
        assert(nargs != 0);                                      \
        Py_INCREF(awaitable);                                    \
        PyAwaitableObject *aw = (PyAwaitableObject *) awaitable; \
        Py_ssize_t final_size = index + nargs;                   \
        if (final_size >= VALUE_ARRAY_SIZE) {                    \
            PyErr_Format(                                        \
    PyExc_SystemError,                                           \
    SAVE_ERR(err),                                               \
    final_size                                                   \
            );                                                   \
            return -1;                                           \
        }                                                        \
        va_list vargs;                                           \
        va_start(vargs, nargs);                                  \
        for (Py_ssize_t i = index; i < final_size; ++i) {        \
            arr[i] = Py_NewRef(va_arg(vargs, tp));               \
        }                                                        \
        index += nargs;                                          \
        va_end(vargs);                                           \
        Py_DECREF(awaitable);                                    \
        return 0

/* Normal Values */

int
pyawaitable_unpack_impl(PyObject *awaitable, ...)
{
    UNPACK(aw->aw_values, PyObject * *, "values");
}

int
pyawaitable_save_impl(PyObject *awaitable, Py_ssize_t nargs, ...)
{
    SAVE(aw->aw_values, aw->aw_values_index, PyObject *, "values");
}

/* Arbitrary Values */

int
pyawaitable_unpack_arb_impl(PyObject *awaitable, ...)
{
    UNPACK(aw->aw_arb_values, void **, "arbitrary values");
}

int
pyawaitable_save_arb_impl(PyObject *awaitable, Py_ssize_t nargs, ...)
{
    SAVE(
        aw->aw_arb_values,
        aw->aw_arb_values_index,
        void *,
        "arbitrary values"
    );
}
