Feature additions:
* `urEnqueue*` functions no longer block unless explicitly told to.
* `urEnqueue*` functions can now wait for an artbitrary list of events to
  complete before starting execution.
* Implement `urQueueFinish` and `urQueueFlush` entry points.

