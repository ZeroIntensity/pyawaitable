#include <Python.h>
#include <pyawaitable/backport.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/values.h>

static int
async_with_inner(PyObject *aw, PyObject *res)
{
    awaitcallback cb;
    awaitcallback_err err;
    PyObject *exit;
    if (pyawaitable_unpack_arb_impl(aw, &cb, &err) < 0)
        return -1;

    if (pyawaitable_unpack_impl(aw, &exit) < 0)
        return -1;

    Py_INCREF(aw);
    Py_INCREF(res);
    int callback_result = cb(aw, res);
    Py_DECREF(res);
    Py_DECREF(aw);

    if (callback_result < 0)
    {
        PyObject *tp, *val, *tb;
        PyErr_Fetch(&tp, &val, &tb);
        PyErr_NormalizeException(&tp, &val, &tb);

        if (tp == NULL)
        {
            PyErr_SetString(
                PyExc_SystemError,
                "pyawaitable: async with callback returned -1 without exception set"
            );
            return -1;
        }

        PyObject *coro = PyObject_Vectorcall(
            exit,
            (PyObject *[]) { tp, val, tb },
            3,
            NULL
        );
        Py_DECREF(tp);
        Py_DECREF(val);
        Py_DECREF(tb);
        if (coro == NULL)
        {
            // TODO: Should we propagate?
            return -1;
        }

        if (pyawaitable_await_impl(aw, coro, NULL, NULL) < 0)
        {
            Py_DECREF(coro);
            return -1;
        }

        return 0;
    } else
    {
        // OK

    }

    return 0;
}

int
pyawaitable_async_with_impl(
    PyObject *aw,
    PyObject *ctx,
    awaitcallback cb,
    awaitcallback_err err
)
{
    PyObject *with = PyObject_GetAttrString(ctx, "__awith__");
    if (with == NULL)
    {
        PyErr_Format(
            PyExc_TypeError,
            "pyawaitable: %R is not an async context manager (missing __awith__)",
            ctx
        );
        return -1;
    }
    PyObject *exit = PyObject_GetAttrString(ctx, "__aexit__");
    if (exit == NULL)
    {
        Py_DECREF(with);
        PyErr_Format(
            PyExc_TypeError,
            "pyawaitable: %R is not an async context manager (missing __aexit__)",
            ctx
        );
        return -1;
    }

    PyObject *inner_aw = pyawaitable_new_impl();

    if (inner_aw == NULL)
    {
        Py_DECREF(with);
        Py_DECREF(exit);
        return -1;
    }

    if (pyawaitable_save_arb_impl(inner_aw, 2, cb, err) < 0)
    {
        Py_DECREF(with);
        Py_DECREF(exit);
        return -1;
    }

    if (pyawaitable_save_impl(inner_aw, 1, exit) < 0)
    {
        Py_DECREF(exit);
        Py_DECREF(with);
        return -1;
    }

    Py_DECREF(exit);

    PyObject *coro = PyObject_CallNoArgs(with);
    Py_DECREF(with);

    if (coro == NULL)
    {
        Py_DECREF(inner_aw);
        return -1;
    }

    // Note: Errors in __aenter__ are not sent to __aexit__
    if (
        pyawaitable_await_impl(
            inner_aw,
            coro,
            async_with_inner,
            NULL
        ) < 0
    )
    {
        Py_DECREF(inner_aw);
        Py_DECREF(coro);
        return -1;
    }

    Py_DECREF(coro);

    if (pyawaitable_await_impl(aw, inner_aw, NULL, err) < 0)
    {
        Py_DECREF(inner_aw);
        return -1;
    }

    Py_DECREF(inner_aw);
    return 0;
}
