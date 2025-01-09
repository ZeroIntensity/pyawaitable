#include <Python.h>
#include <pyawaitable/backport.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/genwrapper.h>
#include <stdlib.h>
#define DONE(cb)                 \
        do { cb->done = true;    \
             Py_CLEAR(cb->coro); \
             Py_CLEAR(g->gw_current_await); } while (0)
#define AW_DONE()               \
        do {                    \
            aw->aw_done = true; \
            Py_CLEAR(g->gw_aw); \
        } while (0)
#define DONE_IF_OK(cb)    \
        if (cb != NULL) { \
            DONE(cb);     \
        }

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

static int
genwrapper_traverse(PyObject *self, visitproc visit, void *arg)
{
    GenWrapperObject *gw = (GenWrapperObject *) self;
    Py_VISIT(gw->gw_current_await);
    Py_VISIT(gw->gw_aw);
    return 0;
}

static int
genwrapper_clear(PyObject *self)
{
    GenWrapperObject *gw = (GenWrapperObject *) self;
    Py_CLEAR(gw->gw_current_await);
    Py_CLEAR(gw->gw_aw);
    return 0;
}

static void
gen_dealloc(PyObject *self)
{
    PyObject_GC_UnTrack(self);
    (void)genwrapper_clear(self);
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
    awaitcallback_err err_callback
)
{
    assert(PyErr_Occurred() != NULL);
    if (err_callback == NULL)
    {
        return -1;
    }

    PyObject *err = PyErr_GetRaisedException();

    Py_INCREF(self);
    int e_res = err_callback(self, err);
    Py_DECREF(self);

    if (e_res < 0)
    {
        // If the res is -1, the error is restored.
        // Otherwise, it is not.
        if (e_res == -1)
        {
            PyErr_SetRaisedException(err);
        } else
        {
            Py_DECREF(err);
        }
        return -1;
    }

    Py_DECREF(err);
    return 0;
}

static inline pyawaitable_callback *
genwrapper_advance(GenWrapperObject *gw)
{
    return pyawaitable_array_GET_ITEM(
        &gw->gw_aw->aw_callbacks,
        gw->gw_aw->aw_state++
    );
}

PyObject *
genwrapper_next(PyObject *self)
{
    GenWrapperObject *g = (GenWrapperObject *)self;
    PyAwaitableObject *aw = g->gw_aw;

    if (!aw)
    {
        PyErr_SetString(
            PyExc_RuntimeError,
            "pyawaitable: genwrapper used after return"
        );
        return NULL;
    }

    pyawaitable_callback *cb;

    if (g->gw_current_await == NULL)
    {
        if (pyawaitable_array_LENGTH(&aw->aw_callbacks) == aw->aw_state)
        {
            PyErr_SetObject(
                PyExc_StopIteration,
                aw->aw_result ? aw->aw_result : Py_None
            );
            AW_DONE();
            return NULL;
        }

        cb = genwrapper_advance(g);
        assert(cb != NULL);
        assert(cb->done == false);
        assert(cb->coro != NULL);

        if (cb->coro == NULL)
        {
            printf(
                "len: %ld, state: %ld\n",
                pyawaitable_array_LENGTH(&aw->aw_callbacks),
                aw->aw_state
            );
        }

        if (cb->callback != NULL && cb->coro == NULL)
        {
            int def_res = ((defer_callback)cb->callback)((PyObject*)aw, NULL);
            if (def_res < -1)
            {
                // -2 or lower denotes that the error should be deferred,
                // regardless of whether a handler is present.
                DONE_IF_OK(cb);
                return NULL;
            }

            if (def_res < 0 && !PyErr_Occurred())
            {
                PyErr_SetString(
                    PyExc_SystemError,
                    "pyawaitable: callback returned -1 without exception set"
                );
                DONE_IF_OK(cb);
                return NULL;
            }

            // Callback is done.
            DONE_IF_OK(cb);
            return genwrapper_next(self);
        }

        if (
            Py_TYPE(cb->coro)->tp_as_async == NULL ||
            Py_TYPE(cb->coro)->tp_as_async->am_await == NULL
        )
        {
            PyErr_Format(
                PyExc_TypeError,
                "pyawaitable: %R is not awaitable",
                cb->coro
            );
            DONE(cb);
            AW_DONE();
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
                    cb->err_callback
                ) < 0
            )
            {
                DONE_IF_OK(cb);
                AW_DONE();
                return NULL;
            }

            DONE_IF_OK(cb);
            return genwrapper_next(self);
        }
    } else
    {
        cb = pyawaitable_array_GET_ITEM(&aw->aw_callbacks, aw->aw_state - 1);
    }

    PyObject *result = Py_TYPE(
        g->gw_current_await
    )->tp_iternext(g->gw_current_await);

    if (result != NULL)
    {
        // Yield!
        return result;
    }

    PyObject *occurred = PyErr_Occurred();
    if (!occurred)
    {
        // Coro is done, no result.
        if (!cb->callback)
        {
            // No callback, skip that step.
            DONE(cb);
            return genwrapper_next(self);
        }
    }

    // TODO: I wonder if the occurred check is needed here.
    if (
        occurred && !PyErr_ExceptionMatches(PyExc_StopIteration)
    )
    {
        if (
            genwrapper_fire_err_callback(
                (PyObject *) aw,
                cb->err_callback
            ) < 0
        )
        {
            DONE(cb);
            AW_DONE();
            return NULL;
        }

        DONE(cb);
        return genwrapper_next(self);
    }

    if (cb->callback == NULL)
    {
        // Coroutine is done, but with a result.
        // We can disregard the result if theres no callback.
        DONE(cb);
        PyErr_Clear();
        return genwrapper_next(self);
    }

    // Deduce the return value of the coroutine
    PyObject *value;
    if (occurred)
    {
        value = PyErr_GetRaisedException();
        assert(value != NULL);
        assert(PyObject_IsInstance(value, PyExc_StopIteration));
        PyObject *tmp = PyObject_GetAttrString(value, "value");
        if (tmp == NULL)
        {
            Py_DECREF(value);
            DONE(cb);
            AW_DONE();
            return NULL;
        }
        Py_DECREF(value);
        value = tmp;
    } else
    {
        value = Py_NewRef(Py_None);
    }

    // Preserve the error callback in case we get cancelled
    awaitcallback_err err_callback = cb->err_callback;
    Py_INCREF(aw);
    int res = cb->callback((PyObject *) aw, value);
    Py_DECREF(aw);
    Py_DECREF(value);

    // If we recently cancelled, then cb is no longer valid
    if (aw->aw_recently_cancelled)
    {
        cb = NULL;
    }

    if (res < -1)
    {
        // -2 or lower denotes that the error should be deferred,
        // regardless of whether a handler is present.
        DONE_IF_OK(cb);
        AW_DONE();
        return NULL;
    }

    if (res < 0)
    {
        if (!PyErr_Occurred())
        {
            PyErr_SetString(
                PyExc_RuntimeError,
                "pyawaitable: user callback returned -1 without exception set"
            );
            DONE(cb);
            AW_DONE();
            return NULL;
        }
        if (
            genwrapper_fire_err_callback(
                (PyObject *) aw,
                err_callback
            ) < 0
        )
        {
            DONE_IF_OK(cb);
            AW_DONE();
            return NULL;
        }
    }

    DONE_IF_OK(cb);
    return genwrapper_next(self);
}

PyTypeObject _PyAwaitableGenWrapperType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_genwrapper",
    .tp_basicsize = sizeof(GenWrapperObject),
    .tp_dealloc = gen_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = genwrapper_next,
    .tp_clear = genwrapper_clear,
    .tp_traverse = genwrapper_traverse,
    .tp_new = gen_new,
};
