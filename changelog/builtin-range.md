Bug fixes:

* The `compiler::utils::EncodeBuiltinRangeMetadataPass` no longer sets the
  maximum global size based on the device's `max_concurrent_work_items` value,
  which refers to the *local* size. The pass now accepts a 3D values of global
  sizes, though those are not currently set by the compiler itself.
