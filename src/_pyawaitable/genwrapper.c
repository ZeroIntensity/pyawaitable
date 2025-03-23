#include <Python.h>
#include <pyawaitable/backport.h>
#include <pyawaitable/awaitableobject.h>
#include <pyawaitable/genwrapper.h>
#include <pyawaitable/optimize.h>
#include <pyawaitable/init.h>
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
#define DONE_IF_OK(cb)                        \
        if (PyAwaitable_LIKELY(cb != NULL)) { \
            DONE(cb);                         \
        }
#define DONE_IF_OK_AND_CHECK(cb)                               \
        if (PyAwaitable_UNLIKELY(aw->aw_recently_cancelled)) { \
            cb = NULL;                                         \
        }                                                      \
        else {                                                 \
            DONE(cb);                                          \
        }
/* If we recently cancelled, then cb is no longer valid */
#define CLEAR_CALLBACK_IF_CANCELLED()                          \
        if (PyAwaitable_UNLIKELY(aw->aw_recently_cancelled)) { \
            cb = NULL;                                         \
        }                                                      \

#define FIRE_ERROR_CALLBACK_AND_NEXT()      \
        if (                                \
    _PyAwaitableGenWrapper_FireErrCallback( \
    (PyObject *) aw,                        \
    cb->err_callback                        \
    ) < 0                                   \
        ) {                                 \
            DONE_IF_OK_AND_CHECK(cb);       \
            AW_DONE();                      \
            return NULL;                    \
        }                                   \
        DONE_IF_OK_AND_CHECK(cb);           \
        return _PyAwaitableGenWrapper_Next(self);
#define RETURN_ADVANCE_GENERATOR() \
        DONE_IF_OK(cb);            \
        PyAwaitable_MUSTTAIL return _PyAwaitableGenWrapper_Next(self);

static PyObject *
gen_new(PyTypeObject *tp, PyObject *args, PyObject *kwds)
{
    assert(tp != NULL);
    assert(tp->tp_alloc != NULL);

    PyObject *self = tp->tp_alloc(tp, 0);
    if (PyAwaitable_UNLIKELY(self == NULL)) {
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

_PyAwaitable_INTERNAL(PyObject *)
genwrapper_new(PyAwaitableObject * aw)
{
    assert(aw != NULL);
    PyTypeObject *type = _PyAwaitable_GetGenWrapperType();
    if (PyAwaitable_UNLIKELY(type == NULL)) {
        return NULL;
    }
    GenWrapperObject *g = (GenWrapperObject *) gen_new(
        type,
        NULL,
        NULL
    );

    if (PyAwaitable_UNLIKELY(g == NULL)) {
        return NULL;
    }

    g->gw_aw = (PyAwaitableObject *) Py_NewRef((PyObject *) aw);
    return (PyObject *) g;
}

_PyAwaitable_INTERNAL(int)
_PyAwaitableGenWrapper_FireErrCallback(
    PyObject * self,
    PyAwaitable_Error err_callback
)
{
    assert(PyErr_Occurred() != NULL);
    if (err_callback == NULL) {
        return -1;
    }

    PyObject *err = PyErr_GetRaisedException();
    if (PyAwaitable_UNLIKELY(err == NULL)) {
        PyErr_SetString(
            PyExc_SystemError,
            "PyAwaitable: Something returned -1 without an exception set"
        );
        return -1;
    }

    Py_INCREF(self);
    int e_res = err_callback(self, err);
    Py_DECREF(self);

    if (e_res < 0) {
        // If the res is -1, the error is restored.
        // Otherwise, it is not.
        if (e_res == -1) {
            PyErr_SetRaisedException(err);
        }
        else {
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

static PyObject *
get_generator_return_value(void)
{
    PyObject *value;
    if (PyErr_Occurred()) {
        value = PyErr_GetRaisedException();
        assert(value != NULL);
        assert(PyObject_IsInstance(value, PyExc_StopIteration));
        PyObject *tmp = PyObject_GetAttrString(value, "value");
        if (PyAwaitable_UNLIKELY(tmp == NULL)) {
            Py_DECREF(value);
            return NULL;
        }
        Py_DECREF(value);
        value = tmp;
    }
    else {
        value = Py_NewRef(Py_None);
    }

    return value;
}

static int
maybe_set_result(PyAwaitableObject *aw)
{
    if (pyawaitable_array_LENGTH(&aw->aw_callbacks) == aw->aw_state) {
        PyErr_SetObject(
            PyExc_StopIteration,
            aw->aw_result ? aw->aw_result : Py_None
        );
        return 1;
    }

    return 0;
}

static inline PyAwaitable_COLD PyObject *
bad_callback(void)
{
    PyErr_SetString(
        PyExc_SystemError,
        "PyAwaitable: User callback returned -1 without exception set"
    );
    return NULL;
}

static inline PyObject *
get_awaitable_iterator(PyObject *op)
{
    if (
        PyAwaitable_UNLIKELY(
            Py_TYPE(op)->tp_as_async == NULL ||
            Py_TYPE(op)->tp_as_async->am_await == NULL
        )
    ) {
        // Fall back to the dunder
        // XXX Is this case possible?
        PyObject *__await__ = PyObject_GetAttrString(op, "__await__");
        if (__await__ == NULL) {
            return NULL;
        }

        PyObject *res = PyObject_CallNoArgs(__await__);
        Py_DECREF(__await__);
        return res;
    }

    return Py_TYPE(op)->tp_as_async->am_await(op);
}

_PyAwaitable_INTERNAL(PyObject *) PyAwaitable_HOT
_PyAwaitableGenWrapper_Next(PyObject *self)
{
    GenWrapperObject *g = (GenWrapperObject *)self;
    PyAwaitableObject *aw = g->gw_aw;

    if (PyAwaitable_UNLIKELY(aw == NULL)) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "PyAwaitable: Generator cannot be awaited after returning"
        );
        return NULL;
    }

    pyawaitable_callback *cb;

    if (g->gw_current_await == NULL) {
        if (maybe_set_result(aw)) {
            // Coroutine is done, woohoo!
            AW_DONE();
            return NULL;
        }

        cb = genwrapper_advance(g);
        assert(cb != NULL);
        assert(cb->done == false);

        if (cb->callback != NULL && cb->coro == NULL) {
            int def_res = ((PyAwaitable_Defer)cb->callback)((PyObject *)aw);
            CLEAR_CALLBACK_IF_CANCELLED();
            if (def_res < 0) {
                DONE_IF_OK(cb);
                AW_DONE();
                return NULL;
            }

            // Callback is done.
            RETURN_ADVANCE_GENERATOR();
        }

        assert(cb->coro != NULL);
        g->gw_current_await = get_awaitable_iterator(cb->coro);
        if (g->gw_current_await == NULL) {
            FIRE_ERROR_CALLBACK_AND_NEXT();
        }
    }
    else {
        cb = pyawaitable_array_GET_ITEM(&aw->aw_callbacks, aw->aw_state - 1);
    }

    PyObject *result = Py_TYPE(
        g->gw_current_await
    )->tp_iternext(g->gw_current_await);

    if (result != NULL) {
        // Yield!
        return result;
    }

    // Rare, but it's possible that the generator cancelled us
    CLEAR_CALLBACK_IF_CANCELLED();

    PyObject *occurred = PyErr_Occurred();
    if (!occurred) {
        // Coro is done, no result.
        if (cb == NULL || !cb->callback) {
            // No callback, skip trying to handle anything
            RETURN_ADVANCE_GENERATOR();
        }
    }

    if (occurred && !PyErr_ExceptionMatches(PyExc_StopIteration)) {
        // An error occurred!
        FIRE_ERROR_CALLBACK_AND_NEXT();
    }

    // Coroutine is done, but with a non-None result.
    if (cb == NULL || cb->callback == NULL) {
        // We can disregard the result if there's no callback.
        PyErr_Clear();
        RETURN_ADVANCE_GENERATOR();
    }

    assert(cb != NULL);
    // Deduce the return value of the coroutine
    PyObject *value = get_generator_return_value();
    if (value == NULL) {
        DONE(cb);
        AW_DONE();
        return NULL;
    }

    // Preserve the error callback in case we get cancelled
    PyAwaitable_Error err_callback = cb->err_callback;
    Py_INCREF(aw);
    int res = cb->callback((PyObject *) aw, value);
    Py_DECREF(aw);
    Py_DECREF(value);

    CLEAR_CALLBACK_IF_CANCELLED();

    // Sanity check to make sure that there's actually
    // an error set.
    if (res < 0) {
        if (!PyErr_Occurred()) {
            DONE(cb);
            AW_DONE();
            return bad_callback();
        }
    }

    if (res < -1) {
        // -2 or lower denotes that the error should be deferred,
        // regardless of whether a handler is present.
        DONE_IF_OK(cb);
        AW_DONE();
        return NULL;
    }

    if (res < 0) {
        FIRE_ERROR_CALLBACK_AND_NEXT();
    }

    RETURN_ADVANCE_GENERATOR();
}

_PyAwaitable_INTERNAL_DATA_DEF(PyTypeObject) _PyAwaitableGenWrapperType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_PyAwaitableGenWrapperType",
    .tp_basicsize = sizeof(GenWrapperObject),
    .tp_dealloc = gen_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = _PyAwaitableGenWrapper_Next,
    .tp_clear = genwrapper_clear,
    .tp_traverse = genwrapper_traverse,
    .tp_new = gen_new,
};
