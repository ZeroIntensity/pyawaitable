#include <Python.h>
#include <pyawaitable/backport.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/genwrapper.h>
#include <pyawaitable/coro.h>

static PyObject *
awaitable_send_with_arg(PyObject *self, PyObject *value)
{
    PyAwaitableObject *aw = (PyAwaitableObject *) self;
    if (aw->aw_gen == NULL)
    {
        PyObject *gen = awaitable_next(self);
        if (gen == NULL)
            return NULL;

        Py_RETURN_NONE;
    }

    return genwrapper_next(aw->aw_gen);
}

static PyObject *
awaitable_send(PyObject *self, PyObject *args)
{
    PyObject *value;

    if (!PyArg_ParseTuple(args, "O", &value))
        return NULL;

    return awaitable_send_with_arg(self, value);
}

static PyObject *
awaitable_close(PyObject *self, PyObject *args)
{
    pyawaitable_cancel_impl(self);
    PyAwaitableObject *aw = (PyAwaitableObject *) self;
    aw->aw_done = true;
    Py_RETURN_NONE;
}

static PyObject *
awaitable_throw(PyObject *self, PyObject *args)
{
    PyObject *type;
    PyObject *value = NULL;
    PyObject *traceback = NULL;

    if (!PyArg_ParseTuple(args, "O|OO", &type, &value, &traceback))
        return NULL;

    if (PyType_Check(type))
    {
        PyObject *err = PyObject_Vectorcall(
            type,
            (PyObject *[]) { value },
            1,
            NULL
        );
        if (err == NULL)
        {
            return NULL;
        }

        if (traceback)
            if (PyException_SetTraceback(err, traceback) < 0)
            {
                Py_DECREF(err);
                return NULL;
            }

        PyErr_Restore(err, NULL, NULL);
    } else
        PyErr_Restore(
            Py_NewRef(type),
            Py_XNewRef(value),
            Py_XNewRef(traceback)
        );

    PyAwaitableObject *aw = (PyAwaitableObject *) self;
    if ((aw->aw_gen != NULL) && (aw->aw_state != 0))
    {
        GenWrapperObject *gw = (GenWrapperObject *) aw->aw_gen;
        pyawaitable_callback *cb = aw->aw_callbacks[aw->aw_state - 1];
        if (cb == NULL)
            return NULL;

        if (genwrapper_fire_err_callback(self, gw->gw_current_await, cb) < 0)
            return NULL;
    } else
        return NULL;

    assert(NULL);
}

#if PY_MINOR_VERSION > 9
static PySendResult
awaitable_am_send(PyObject *self, PyObject *arg, PyObject **presult)
{
    PyObject *send_res = awaitable_send_with_arg(self, arg);
    if (send_res == NULL)
    {
        PyObject *occurred = PyErr_Occurred();
        if (PyErr_GivenExceptionMatches(occurred, PyExc_StopIteration))
        {
            PyObject *item = PyObject_GetAttrString(occurred, "value");

            if (item == NULL)
            {
                return PYGEN_ERROR;
            }

            *presult = item;
            return PYGEN_RETURN;
        }
        *presult = NULL;
        return PYGEN_ERROR;
    }
    PyAwaitableObject *aw = (PyAwaitableObject *) self;
    *presult = send_res;

    return PYGEN_NEXT;
}

#endif

PyMethodDef pyawaitable_methods[] =
{
    {"send", awaitable_send, METH_VARARGS, NULL},
    {"close", awaitable_close, METH_VARARGS, NULL},
    {"throw", awaitable_throw, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}
};

PyAsyncMethods pyawaitable_async_methods =
{
    #if PY_MINOR_VERSION > 9
    .am_await = awaitable_next,
    .am_send = awaitable_am_send
    #else
    .am_await = awaitable_next
    #endif
};
