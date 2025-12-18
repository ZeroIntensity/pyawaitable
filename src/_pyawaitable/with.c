#include <Python.h>
#include <pyawaitable/backport.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/values.h>

static int
async_with_inner(PyObject *aw, PyObject *res)
{
    PyAwaitable_Callback cb;
    PyAwaitable_Error err;
    PyObject *exit;
    if (PyAwaitable_UnpackArbValues(aw, &cb, &err) < 0) {
        return -1;
    }

    if (PyAwaitable_UnpackValues(aw, &exit) < 0) {
        return -1;
    }

    Py_INCREF(aw);
    Py_INCREF(res);
    int callback_result = cb != NULL ? cb(aw, res) : 0;
    Py_DECREF(res);
    Py_DECREF(aw);

    PyObject *args[3];
    if (callback_result < 0) {
        PyObject *tp, *val, *tb;
        PyErr_Fetch(&tp, &val, &tb);
        PyErr_NormalizeException(&tp, &val, &tb);

        if (tp == NULL) {
            PyErr_SetString(
                PyExc_SystemError,
                "PyAwaitable: `async with` callback returned -1 without exception set"
            );
            return -1;
        }

        // Traceback can still be NULL
        if (tb == NULL)
            tb = Py_NewRef(Py_None);

        args[0] = tp;
        args[1] = val;
        args[2] = tb;
    }
    else {
        // OK
        args[0] = Py_NewRef(Py_None);
        args[1] = Py_NewRef(Py_None);
        args[2] = Py_NewRef(Py_None);
    }

    int result = PyAwaitable_AddExpr(
        aw,
        PyObject_Vectorcall(
            exit,
            args,
            3,
            NULL),
        NULL,
        NULL);
    Py_DECREF(args[0]);
    Py_DECREF(args[1]);
    Py_DECREF(args[2]);
    if (result < 0) {
        return -1;
    }
    return 0;
}

_PyAwaitable_API(int)
PyAwaitable_AsyncWith(
    PyObject * aw,
    PyObject * ctx,
    PyAwaitable_Callback cb,
    PyAwaitable_Error err
)
{
    PyObject *with = PyObject_GetAttrString(ctx, "__aenter__");
    if (with == NULL) {
        PyErr_Format(
            PyExc_TypeError,
            "PyAwaitable: %R is not an async context manager (missing __aenter__)",
            ctx
        );
        return -1;
    }
    PyObject *exit = PyObject_GetAttrString(ctx, "__aexit__");
    if (exit == NULL) {
        Py_DECREF(with);
        PyErr_Format(
            PyExc_TypeError,
            "PyAwaitable: %R is not an async context manager (missing __aexit__)",
            ctx
        );
        return -1;
    }

    PyObject *inner_aw = PyAwaitable_New();

    if (inner_aw == NULL) {
        Py_DECREF(with);
        Py_DECREF(exit);
        return -1;
    }

    if (PyAwaitable_SaveArbValues(inner_aw, 2, cb, err) < 0) {
        Py_DECREF(inner_aw);
        Py_DECREF(with);
        Py_DECREF(exit);
        return -1;
    }

    if (PyAwaitable_SaveValues(inner_aw, 1, exit) < 0) {
        Py_DECREF(inner_aw);
        Py_DECREF(exit);
        Py_DECREF(with);
        return -1;
    }

    Py_DECREF(exit);

    // Note: Errors in __aenter__ are not sent to __aexit__
    if (
        PyAwaitable_AddExpr(
            inner_aw,
            PyObject_CallNoArgs(with),
            async_with_inner,
            NULL
        ) < 0
    ) {
        Py_DECREF(inner_aw);
        Py_DECREF(with);
        return -1;
    }

    Py_DECREF(with);

    if (PyAwaitable_AddExpr(aw, inner_aw, NULL, err) < 0) {
        return -1;
    }

    return 0;
}
