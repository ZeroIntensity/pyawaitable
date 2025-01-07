#include <Python.h>
#include <stdarg.h>

#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/backport.h>
#include <pyawaitable/array.h>
#include <pyawaitable/values.h>

#define NOTHING

#define SAVE(field, type, extra)                                    \
        PyAwaitableObject *aw = (PyAwaitableObject *) awaitable;    \
        pyawaitable_array *array = &aw->field;                      \
        va_list vargs;                                              \
        va_start(vargs, nargs);                                     \
        for (Py_ssize_t i = 0; i < nargs; ++i) {                    \
            type ptr = va_arg(vargs, type);                         \
            assert((void *)ptr != NULL);                            \
            if (pyawaitable_array_append(array, (void *)ptr) < 0) { \
                PyErr_NoMemory();                                   \
                return -1;                                          \
            }                                                       \
            extra;                                                  \
        }                                                           \
        va_end(vargs);                                              \
        return 0

#define UNPACK(field, type)                                                \
        PyAwaitableObject *aw = (PyAwaitableObject *) awaitable;           \
        pyawaitable_array *array = &aw->field;                             \
        if (pyawaitable_array_LENGTH(array) == 0) {                        \
            PyErr_SetString(                                               \
    PyExc_RuntimeError,                                                    \
    "pyawaitable: object has no stored values"                             \
            );                                                             \
            return -1;                                                     \
        }                                                                  \
        va_list vargs;                                                     \
        va_start(vargs, awaitable);                                        \
        for (Py_ssize_t i = 0; i < pyawaitable_array_LENGTH(array); ++i) { \
            type *ptr = va_arg(vargs, type *);                             \
            if (ptr == NULL) {                                             \
                continue;                                                  \
            }                                                              \
            *ptr = (type)pyawaitable_array_GET_ITEM(array, i);             \
        }                                                                  \
        va_end(vargs);                                                     \
        return 0

#define SET(field, type)                                          \
        assert(awaitable != NULL);                                \
        PyAwaitableObject *aw = (PyAwaitableObject *) awaitable;  \
        pyawaitable_array *array = &aw->field;                    \
        if (check_index(index, array) < 0) {                      \
            return -1;                                            \
        }                                                         \
        pyawaitable_array_set(array, index, (void *)(new_value)); \
        return 0

#define GET(field, type)                                         \
        assert(awaitable != NULL);                               \
        PyAwaitableObject *aw = (PyAwaitableObject *) awaitable; \
        pyawaitable_array *array = &aw->field;                   \
        if (check_index(index, array) < 0) {                     \
            return (type)NULL;                                   \
        }                                                        \
        return (type)pyawaitable_array_GET_ITEM(array, index)

static int
check_index(Py_ssize_t index, pyawaitable_array *array)
{
    assert(array != NULL);
    if (index < 0)
    {
        PyErr_SetString(
            PyExc_IndexError,
            "pyawaitable: cannot set negative index"
        );
        return -1;
    }

    if (index >= pyawaitable_array_LENGTH(array))
    {
        PyErr_SetString(
            PyExc_IndexError,
            "pyawaitable: cannot set index that is out of bounds"
        );
        return -1;
    }

    return 0;
}

int
pyawaitable_unpack_impl(PyObject *awaitable, ...)
{
    UNPACK(aw_object_values, PyObject *);
}

int
pyawaitable_save_impl(PyObject *awaitable, Py_ssize_t nargs, ...)
{
    SAVE(aw_object_values, PyObject *, Py_INCREF(ptr));
}

int
pyawaitable_set_impl(
    PyObject *awaitable,
    Py_ssize_t index,
    PyObject *new_value
)
{
    SET(aw_object_values, Py_NewRef);
}

PyObject *
pyawaitable_get_impl(
    PyObject *awaitable,
    Py_ssize_t index
)
{
    GET(aw_object_values, PyObject *);
}

/* Arbitrary Values */

int
pyawaitable_unpack_arb_impl(PyObject *awaitable, ...)
{
    UNPACK(aw_arbitrary_values, void *);
}

int
pyawaitable_save_arb_impl(PyObject *awaitable, Py_ssize_t nargs, ...)
{
    SAVE(aw_arbitrary_values, void *, NOTHING);
}

int
pyawaitable_set_arb_impl(
    PyObject *awaitable,
    Py_ssize_t index,
    void *new_value
)
{
    SET(aw_arbitrary_values, void *);
}

void *
pyawaitable_get_arb_impl(
    PyObject *awaitable,
    Py_ssize_t index
)
{
    GET(aw_arbitrary_values, void *);
}

/* Integer Values */

int
pyawaitable_unpack_int_impl(PyObject *awaitable, ...)
{
    UNPACK(aw_integer_values, long);
}

int
pyawaitable_save_int_impl(PyObject *awaitable, Py_ssize_t nargs, ...)
{
    SAVE(aw_integer_values, long, NOTHING);
}

int
pyawaitable_set_int_impl(
    PyObject *awaitable,
    Py_ssize_t index,
    long new_value
)
{
    SET(aw_integer_values, long);
}

long
pyawaitable_get_int_impl(
    PyObject *awaitable,
    Py_ssize_t index
)
{
    GET(aw_integer_values, long);
}
