#ifndef PYAWAITABLE__PYAWAITABLE_H
#define PYAWAITABLE__PYAWAITABLE_H

#include <Python.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>

#include "array.h"
#include "awaitableobject.h"
#include "backport.h"
#include "genwrapper.h"
#include "coro.h"
#include "values.h"
#include "with.h"

#define PYAWAITABLE_BEING_BUILT
#include <pyawaitable.h>

#endif
