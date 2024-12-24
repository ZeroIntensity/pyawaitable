#include <Python.h>
#include <stdarg.h>

#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/backport.h>
#include <pyawaitable/array.h>
#include <pyawaitable/values.h>

#define SAVE(field, type, extra)                                   \
        PyAwaitableObject *aw = (PyAwaitableObject *) awaitable;   \
        pyawaitable_array *array = &aw->field;                     \
        va_list vargs;                                             \
        va_start(vargs, nargs);                                    \
        for (Py_ssize_t i = 0; i < nargs; ++i) {                   \
            type ptr = va_arg(vargs, type);                        \
            assert(ptr != NULL);                                   \
            if (pyawaitable_array_append(array, (type) ptr) < 0) { \
                PyErr_NoMemory();                                  \
                return -1;                                         \
            }                                                      \
            extra;                                                 \
        }                                                          \
        va_end(vargs);                                             \
        return 0

#define UNPACK(field, type)                                                \
        PyAwaitableObject *aw = (PyAwaitableObject *) awaitable;           \
        pyawaitable_array *array = &aw->field;                             \
        if (pyawaitable_array_LENGTH(array) == 0) {                        \
            PyErr_SetString(                                               \
    PyExc_SystemError,                                                     \
    "pyawaitable: object has no stored values"                             \
            );                                                             \
            return -1;                                                     \
        }                                                                  \
        va_list vargs;                                                     \
        va_start(vargs, awaitable);                                        \
        for (Py_ssize_t i = 0; i < pyawaitable_array_LENGTH(array); ++i) { \
            type *ptr = va_arg(vargs, type *);                             \
            *ptr = pyawaitable_array_GET_ITEM(array, i);                   \
        }                                                                  \
        va_end(vargs);                                                     \
        return 0

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

static int
check_index(Py_ssize_t index, pyawaitable_array *array)
{
    assert(array != NULL);
    if (index < 0)
    {
        PyErr_SetString(
            PyExc_SystemError,
            "pyawaitable: cannot set negative index"
        );
        return -1;
    }

    if (index >= pyawaitable_array_LENGTH(array))
    {
        PyErr_SetString(
            PyExc_SystemError,
            "pyawaitable: cannot set index that is out of bounds"
        );
        return -1;
    }

    return 0;
}

int
pyawaitable_set_impl(
    PyObject *awaitable,
    Py_ssize_t index,
    PyObject *new_value
)
{
    assert(awaitable != NULL);
    assert(new_value != NULL);
    PyAwaitableObject *aw = (PyAwaitableObject *) awaitable;
    pyawaitable_array *array = &aw->aw_object_values;
    if (check_index(index, array) < 0)
    {
        return -1;
    }
    pyawaitable_array_set(array, index, (PyObject *)Py_NewRef(new_value));
    return 0;
}

PyObject *
pyawaitable_get_impl(
    PyObject *awaitable,
    Py_ssize_t index
)
{
    assert(awaitable != NULL);
    PyAwaitableObject *aw = (PyAwaitableObject *) awaitable;
    pyawaitable_array *array = &aw->aw_object_values;
    if (check_index(index, array) < 0)
    {
        return NULL;
    }
    return pyawaitable_array_GET_ITEM(array, index);
}

/* Arbitrary Values */

int
pyawaitable_unpack_arb_impl(PyObject *awaitable, ...)
{
    UNPACK(
        aw->aw_arb_values,
        void **,
        "arbitrary values",
        aw->aw_arb_values_index
    );
}

int
pyawaitable_save_arb_impl(PyObject *awaitable, Py_ssize_t nargs, ...)
{
    SAVE(
        aw->aw_arb_values,
        aw->aw_arb_values_index,
        void *,
        "arbitrary values",
        NOTHING
    );
}

int
pyawaitable_set_arb_impl(
    PyObject *awaitable,
    Py_ssize_t index,
    void *new_value
)
{
    INDEX_HEAD(aw->aw_arb_values, aw->aw_arb_values_index, -1);
    aw->aw_arb_values[index] = new_value;
    return 0;
}

void *
pyawaitable_get_arb_impl(
    PyObject *awaitable,
    Py_ssize_t index
)
{
    INDEX_HEAD(aw->aw_arb_values, aw->aw_arb_values_index, NULL);
    return aw->aw_arb_values[index];
}

/* Integer Values */

int
pyawaitable_unpack_int_impl(PyObject *awaitable, ...)
{
    UNPACK(
        aw->aw_int_values,
        long *,
        "integer values",
        aw->aw_int_values_index
    );
}

int
pyawaitable_save_int_impl(PyObject *awaitable, Py_ssize_t nargs, ...)
{
    SAVE(
        aw->aw_int_values,
        aw->aw_int_values_index,
        long,
        "integer values",
        NOTHING
    );
}

int
pyawaitable_set_int_impl(
    PyObject *awaitable,
    Py_ssize_t index,
    long new_value
)
{
    INDEX_HEAD(aw->aw_int_values, aw->aw_int_values_index, -1);
    aw->aw_int_values[index] = new_value;
    return 0;
}

long
pyawaitable_get_int_impl(
    PyObject *awaitable,
    Py_ssize_t index
)
{
    INDEX_HEAD(aw->aw_int_values, aw->aw_int_values_index, -1);
    return aw->aw_int_values[index];
}
