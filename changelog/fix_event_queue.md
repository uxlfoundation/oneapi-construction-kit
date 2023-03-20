Bug fixes:
* Fixed issue with non-user event dependencies across queues for OpenCL. This is
  a fix for the following case:
    Queue 1 : clEnqueueNDRange -> event_kernel
    Queue 2 : clEnqueueReadBuffer <- wait event_kernel, -> event_read
    clWaitForEvents(event_read)
   This involves flushing the dependent event's queue and waiting for the event.
   This should all be internal and require no user changes.
