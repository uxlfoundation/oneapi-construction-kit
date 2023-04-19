Bug fixes:
* Fix for clReleaseCommandQueue leaving event functions in a bad state. The fix
  involves retaining the command queue in the event and also adding an
  effective finish when clReleaseCommandQueue is called, when we have events
  waiting. Additionally wait events have to be retained until they are cleaned
  up once they are removed from the dispatch list. This fixes a number of
  issues including CTS fail test_api/queue_flush_on_release,
  and vexcl/events test.
