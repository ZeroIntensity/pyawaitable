#include <Python.h>
#include <stdarg.h>

#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/backport.h>
#include <pyawaitable/array.h>
#include <pyawaitable/values.h>
#include <pyawaitable/optimize.h>

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
    "PyAwaitable: Object has no stored values"                             \
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
    if (PyAwaitable_UNLIKELY(index < 0)) {
        PyErr_SetString(
            PyExc_IndexError,
            "PyAwaitable: Cannot set negative index"
        );
        return -1;
    }

    if (PyAwaitable_UNLIKELY(index >= pyawaitable_array_LENGTH(array))) {
        PyErr_SetString(
            PyExc_IndexError,
            "PyAwaitable: Cannot set index that is out of bounds"
        );
        return -1;
    }

    return 0;
}

_PyAwaitable_API(int)
PyAwaitable_UnpackValues(PyObject * awaitable, ...)
{
    UNPACK(aw_object_values, PyObject *);
}

_PyAwaitable_API(int)
PyAwaitable_SaveValues(PyObject * awaitable, Py_ssize_t nargs, ...)
{
    SAVE(aw_object_values, PyObject *, Py_INCREF(ptr));
}

_PyAwaitable_API(int)
PyAwaitable_SetValue(
    PyObject * awaitable,
    Py_ssize_t index,
    PyObject * new_value
)
{
    SET(aw_object_values, Py_NewRef);
}

_PyAwaitable_API(PyObject *)
PyAwaitable_GetValue(
    PyObject * awaitable,
    Py_ssize_t index
)
{
    GET(aw_object_values, PyObject *);
}

/* Arbitrary Values */

_PyAwaitable_API(int)
PyAwaitable_UnpackArbValues(PyObject * awaitable, ...)
{
    UNPACK(aw_arbitrary_values, void *);
}

_PyAwaitable_API(int)
PyAwaitable_SaveArbValues(PyObject * awaitable, Py_ssize_t nargs, ...)
{
    SAVE(aw_arbitrary_values, void *, NOTHING);
}

_PyAwaitable_API(int)
PyAwaitable_SetArbValue(
    PyObject * awaitable,
    Py_ssize_t index,
    void *new_value
)
{
    SET(aw_arbitrary_values, void *);
}

_PyAwaitable_API(void *)
PyAwaitable_GetArbValue(
    PyObject * awaitable,
    Py_ssize_t index
)
{
    GET(aw_arbitrary_values, void *);
}
