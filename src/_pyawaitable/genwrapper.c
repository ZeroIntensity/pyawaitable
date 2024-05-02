/* *INDENT-OFF* */
#include <Python.h>
#include <pyawaitable/backport.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/genwrapper.h>

static PyObject *
gen_new(PyTypeObject *tp, PyObject *args, PyObject *kwds)
{
    assert(tp != NULL);
    assert(tp->tp_alloc != NULL);

    PyObject *self = tp->tp_alloc(tp, 0);
    if (self == NULL) {
        return NULL;
    }

    GenWrapperObject *g = (GenWrapperObject *) self;
    g->gw_result = NULL;
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
    Py_XDECREF(g->gw_result);
    Py_TYPE(self)->tp_free(self);
}

PyObject *
genwrapper_new(AwaitableObject *aw)
{
    assert(aw != NULL);
    GenWrapperObject *g = (GenWrapperObject *) gen_new(
        &_AwaitableGenWrapperType,
        NULL,
        NULL
    );

    if (g == NULL) return NULL;
    g->gw_aw = (AwaitableObject *) Py_NewRef((PyObject *) aw);
    return (PyObject *) g;
}

void
awaitable_genwrapper_set_result(PyObject *gen, PyObject *result)
{
    assert(gen != NULL);
    assert(result != NULL);
    Py_INCREF(gen);
    GenWrapperObject *g = (GenWrapperObject *) gen;

    g->gw_result = Py_NewRef(result);
    Py_DECREF(gen);
}

int
genwrapper_fire_err_callback(PyObject *self, PyObject *await, awaitable_callback *cb)
{
    assert(PyErr_Occurred() != NULL);
    if (!cb->err_callback) {
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

    if (e_res < 0) {
        if (PyErr_Occurred())
            PyErr_Print();

        if (e_res == -1)
            PyErr_SetRaisedException(err);

        // if the res is -1, the error is restored. otherwise, it is not
        Py_DECREF(err);
        Py_DECREF(cb->coro);
        Py_XDECREF(await);
        return -1;
    };

    Py_DECREF(err);

    return 0;
}

PyObject *
genwrapper_next(PyObject *self)
{
    GenWrapperObject *g = (GenWrapperObject *) self;
    AwaitableObject *aw = g->gw_aw;
    awaitable_callback *cb;
    if (((aw->aw_state + 1) > aw->aw_callback_size) &&
        g->gw_current_await == NULL) {
        PyErr_SetObject(PyExc_StopIteration,
                        g->gw_result ?
                        g->gw_result :
                        Py_None);
        return NULL;
    }

    if (g->gw_current_await == NULL) {
        cb = aw->aw_callbacks[aw->aw_state++];

        if (Py_TYPE(cb->coro)->tp_as_async == NULL ||
            Py_TYPE(cb->coro)->tp_as_async->am_await == NULL) {
            PyErr_Format(PyExc_TypeError, "%R has no __await__", cb->coro);
            return NULL;
        }

        g->gw_current_await = Py_TYPE(cb->coro)->tp_as_async->am_await(
                                                            cb->coro);
        if (g->gw_current_await == NULL) {
            if (genwrapper_fire_err_callback((PyObject *) aw, g->gw_current_await, cb) < 0) {
                return NULL;
            }

            return genwrapper_next(self);
        }
    } else {
        cb = aw->aw_callbacks[aw->aw_state - 1];
    }

    PyObject *result = Py_TYPE(g->gw_current_await
                        )->tp_iternext(g->gw_current_await);

    if (result == NULL) {
        PyObject *occurred = PyErr_Occurred();
        if (!occurred) {
            // coro is done
            g->gw_current_await = NULL;
            return genwrapper_next(self);
        }

        if (!PyErr_GivenExceptionMatches(occurred, PyExc_StopIteration)) {
            if (genwrapper_fire_err_callback((PyObject *) aw, g->gw_current_await, cb) < 0) {
                return NULL;
            }
            g->gw_current_await = NULL;
            return genwrapper_next(self);
        }

        if (cb->callback == NULL) {
            // coro is done, but with a result
            // we can disregard the result if theres no callback
            g->gw_current_await = NULL;
            PyErr_Clear();
            return genwrapper_next(self);
        }

        PyObject *type, *value, *traceback;
        PyErr_Fetch(&type, &value, &traceback);
        PyErr_NormalizeException(&type, &value, &traceback);

        if (value == NULL) {
            value = Py_NewRef(Py_None);
        } else {
            Py_INCREF(value);
            PyObject* tmp = PyObject_GetAttrString(value, "value");
            if (tmp == NULL) {
                Py_DECREF(value);
                return NULL;
            }
            Py_DECREF(value);
            value = tmp;
            Py_INCREF(value);
        }

        Py_INCREF(aw);
        int result = cb->callback((PyObject *) aw, value);
        if (result < -1) {
            // -2 or lower denotes that the error should be deferred
            // regardless of whether a handler is present
            return NULL;
        }

        if (result < 0) {
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_SystemError, "callback returned -1 without exception set");
                return NULL;
            }
            if (genwrapper_fire_err_callback((PyObject *) aw, g->gw_current_await, cb) < 0) {
                PyErr_Restore(type, value, traceback);
                return NULL;
            }
        }

        cb->done = true;
        g->gw_current_await = NULL;
        return genwrapper_next(self);
    }

    return result;
}

PyTypeObject _AwaitableGenWrapperType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_genwrapper", 
    .tp_basicsize = sizeof(GenWrapperObject),
    .tp_dealloc = gen_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = genwrapper_next,
    .tp_new = gen_new,
};
