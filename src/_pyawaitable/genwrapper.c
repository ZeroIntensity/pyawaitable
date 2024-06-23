#include <Python.h>
#include <pyawaitable/backport.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/genwrapper.h>
#include <stdlib.h>


static PyObject *
gen_new(PyTypeObject *tp, PyObject *args, PyObject *kwds)
{
    assert(tp != NULL);
    assert(tp->tp_alloc != NULL);

    PyObject *self = tp->tp_alloc(tp, 0);
    if (self == NULL)
    {
        return NULL;
    }

    GenWrapperObject *g = (GenWrapperObject *) self;
    g->gw_aw = NULL;
    g->gw_current_await = NULL;

    return (PyObject *) g;
}

static void
gen_dealloc(PyObject *self)
{
    GenWrapperObject *g = (GenWrapperObject *) self;
    Py_XDECREF(g->gw_current_await);
    Py_XDECREF(g->gw_aw);
    Py_TYPE(self)->tp_free(self);
}

PyObject *
genwrapper_new(PyAwaitableObject *aw)
{
    assert(aw != NULL);
    GenWrapperObject *g = (GenWrapperObject *) gen_new(
        &_PyAwaitableGenWrapperType,
        NULL,
        NULL
    );

    if (!g)
        return NULL;

    g->gw_aw = (PyAwaitableObject *) Py_NewRef((PyObject *) aw);
    return (PyObject *) g;
}

int
genwrapper_fire_err_callback(
    PyObject *self,
    PyObject *await,
    pyawaitable_callback *cb
)
{
    assert(PyErr_Occurred() != NULL);
    if (!cb->err_callback)
    {
        cb->done = true;
        Py_DECREF(cb->coro);
        Py_XDECREF(await);
        return -1;
    }

    Py_INCREF(self);
    PyObject *err = PyErr_GetRaisedException();

    int e_res = cb->err_callback(self, err);
    cb->done = true;

    Py_DECREF(self);

    if (e_res < 0)
    {
        // If the res is -1, the error is restored.
        // Otherwise, it is not.
        if (e_res == -1)
            PyErr_SetRaisedException(err);
        else
            Py_DECREF(err);

        Py_DECREF(cb->coro);
        Py_XDECREF(await);
        return -1;
    }

    Py_DECREF(err);
    return 0;
}

PyObject *
genwrapper_next(PyObject *self)
{
    GenWrapperObject *g = (GenWrapperObject *)self;
    PyAwaitableObject *aw = g->gw_aw;
    pyawaitable_callback *cb;
    if (aw->aw_state == CALLBACK_ARRAY_SIZE)
    {
        PyErr_SetString(
            PyExc_SystemError,
            "pyawaitable cannot handle more than 255 coroutines"
        );
        return NULL;
    }

    if (g->gw_current_await == NULL)
    {
        if (aw->aw_callbacks[aw->aw_state] == NULL)
        {
            aw->aw_done = true;
            PyErr_SetObject(
                PyExc_StopIteration,
                aw->aw_result ? aw->aw_result : Py_None
            );
            return NULL;
        }

        cb = aw->aw_callbacks[aw->aw_state++];

        if (
            Py_TYPE(cb->coro)->tp_as_async == NULL ||
            Py_TYPE(cb->coro)->tp_as_async->am_await == NULL
        )
        {
            PyErr_Format(PyExc_TypeError, "%R has no __await__", cb->coro);
            return NULL;
        }

        g->gw_current_await = Py_TYPE(cb->coro)->tp_as_async->am_await(
            cb->coro
        );
        if (g->gw_current_await == NULL)
        {
            if (
                genwrapper_fire_err_callback(
                    (PyObject *)aw,
                    g->gw_current_await,
                    cb
                ) < 0
            )
            {
                return NULL;
            }

            return genwrapper_next(self);
        }
    } else
    {
        cb = aw->aw_callbacks[aw->aw_state - 1];
    }

    PyObject *result = Py_TYPE(
        g->gw_current_await
    )->tp_iternext(g->gw_current_await);

    if (result == NULL)
    {
        PyObject *occurred = PyErr_Occurred();
        if (!occurred)
        {
            // Coro is done
            if (!cb->callback)
            {
                g->gw_current_await = NULL;
                return genwrapper_next(self);
            }
        }

        if (
            occurred && !PyErr_GivenExceptionMatches(
                occurred,
                PyExc_StopIteration
            )
        )
        {
            if (
                genwrapper_fire_err_callback(
                    (PyObject *) aw,
                    g->gw_current_await,
                    cb
                ) < 0
            )
            {
                return NULL;
            }
            g->gw_current_await = NULL;
            return genwrapper_next(self);
        }

        if (cb->callback == NULL)
        {
            // Coroutine is done, but with a result.
            // We can disregard the result if theres no callback.
            g->gw_current_await = NULL;
            PyErr_Clear();
            return genwrapper_next(self);
        }

        PyObject *value;
        if (occurred)
        {
            PyObject *type, *traceback;
            PyErr_Fetch(&type, &value, &traceback);
            PyErr_NormalizeException(&type, &value, &traceback);
            Py_XDECREF(type);
            Py_XDECREF(traceback);

            if (value == NULL)
            {
                value = Py_NewRef(Py_None);
            } else
            {
                assert(PyObject_IsInstance(value, PyExc_StopIteration));
                PyObject *tmp = PyObject_GetAttrString(value, "value");
                if (tmp == NULL)
                {
                    Py_DECREF(value);
                    return NULL;
                }
                value = tmp;
            }
        } else
        {
            value = Py_NewRef(Py_None);
        }

        Py_INCREF(aw);
        int result = cb->callback((PyObject *) aw, value);
        Py_DECREF(aw);
        Py_DECREF(value);

        if (result < -1)
        {
            // -2 or lower denotes that the error should be deferred,
            // regardless of whether a handler is present.
            return NULL;
        }

        if (result < 0)
        {
            if (!PyErr_Occurred())
            {
                PyErr_SetString(
                    PyExc_SystemError,
                    "pyawaitable: callback returned -1 without exception set"
                );
                return NULL;
            }
            if (
                genwrapper_fire_err_callback(
                    (PyObject *)aw,
                    g->gw_current_await,
                    cb
                ) < 0
            )
            {
                return NULL;
            }
        }

        cb->done = true;
        g->gw_current_await = NULL;
        return genwrapper_next(self);
    }

    return result;
}

PyTypeObject _PyAwaitableGenWrapperType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_genwrapper",
    .tp_basicsize = sizeof(GenWrapperObject),
    .tp_dealloc = gen_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = genwrapper_next,
    .tp_new = gen_new,
};
