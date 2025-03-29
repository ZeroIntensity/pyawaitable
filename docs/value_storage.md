# Managing State Between Callbacks

So far, all of our examples haven't needed any transfer of state between the `PyAwaitable_AddAwait` callsite and the time the coroutine is executed (_i.e._, in a callback). You might have noticed that the callbacks don't take a `void *arg` parameter to make up for C's lack of closures, so what's the best step?
