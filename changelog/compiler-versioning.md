Upgrade guidance:

* The compiler now checks the OpenCL version compatibility of a module at
  runtime in more cases, as opposed to at build time. This is encoded as
  `!opencl.ocl.version` metadata.
