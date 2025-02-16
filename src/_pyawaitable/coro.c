#include <Python.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/backport.h>
#include <pyawaitable/coro.h>
#include <pyawaitable/genwrapper.h>
#include <pyawaitable/optimize.h>

static PyObject *
awaitable_send_with_arg(PyObject *self, PyObject *value)
{
    PyAwaitableObject *aw = (PyAwaitableObject *) self;
    if (aw->aw_gen == NULL) {
        PyObject *gen = awaitable_next(self);
        if (PyAwaitable_UNLIKELY(gen == NULL)) {
            return NULL;
        }

        if (PyAwaitable_UNLIKELY(value != Py_None)) {
            PyErr_SetString(
                PyExc_RuntimeError,
                "can't send non-None value to a just-started awaitable"
            );
            return NULL;
        }

        Py_DECREF(gen);
        Py_RETURN_NONE;
    }

    return _PyAwaitableGenWrapper_Next(aw->aw_gen);
}

static PyObject *
awaitable_send(PyObject *self, PyObject *value)
{
    return awaitable_send_with_arg(self, value);
}

static PyObject *
awaitable_close(PyObject *self, PyObject *args)
{
    PyAwaitable_Cancel(self);
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

    if (!PyArg_ParseTuple(args, "O|OO", &type, &value, &traceback)) {
        return NULL;
    }

    if (PyType_Check(type)) {
        PyObject *err = PyObject_CallOneArg(type, value);
        if (PyAwaitable_UNLIKELY(err == NULL)) {
            return NULL;
        }

        if (traceback != NULL) {
            if (PyException_SetTraceback(err, traceback) < 0) {
                Py_DECREF(err);
                return NULL;
            }
        }

        PyErr_Restore(err, NULL, NULL);
    }
    else {
        PyErr_Restore(
            Py_NewRef(type),
            Py_XNewRef(value),
            Py_XNewRef(traceback)
        );
    }

    PyAwaitableObject *aw = (PyAwaitableObject *)self;
    if ((aw->aw_gen != NULL) && (aw->aw_state != 0)) {
        GenWrapperObject *gw = (GenWrapperObject *)aw->aw_gen;
        pyawaitable_callback *cb =
            pyawaitable_array_GET_ITEM(&aw->aw_callbacks, aw->aw_state - 1);
        if (cb == NULL) {
            return NULL;
        }

        if (_PyAwaitableGenWrapper_FireErrCallback(
            self,
            cb->err_callback
            ) < 0) {
            return NULL;
        }
    }
    else {
        return NULL;
    }

    assert(PyErr_Occurred());
    return NULL;
}

#if PY_MINOR_VERSION > 9
static PySendResult
awaitable_am_send(PyObject *self, PyObject *arg, PyObject **presult)
{
    PyObject *send_res = awaitable_send_with_arg(self, arg);
    if (send_res == NULL) {
        if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
            PyObject *occurred = PyErr_GetRaisedException();
            PyObject *item = PyObject_GetAttrString(occurred, "value");
            Py_DECREF(occurred);

            if (PyAwaitable_UNLIKELY(item == NULL)) {
                return PYGEN_ERROR;
            }

            *presult = item;
            return PYGEN_RETURN;
        }
        *presult = NULL;
        return PYGEN_ERROR;
    }
    *presult = send_res;

    return PYGEN_NEXT;
}

#endif

_PyAwaitable_INTERNAL_DATA_DEF(PyMethodDef) pyawaitable_methods[] = {
    {"send", awaitable_send, METH_O, NULL},
    {"close", awaitable_close, METH_NOARGS, NULL},
    {"throw", awaitable_throw, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}
};

_PyAwaitable_INTERNAL_DATA_DEF(PyAsyncMethods) pyawaitable_async_methods = {
#if PY_MINOR_VERSION > 9
    .am_await = awaitable_next,
    .am_send = awaitable_am_send
#else
    .am_await = awaitable_next
#endif
};
