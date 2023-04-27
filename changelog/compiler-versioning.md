Upgrade guidance:

* The compiler is now always built in what was previously 3.0 mode.
* The compiler now checks the OpenCL version compatibility of a module at
  runtime, as opposed to at build time. This is encoded as
  `!opencl.ocl.version` metadata.
* `compiler::utils::CLBuiltinInfo` now only identifies builtins when the
  module's OpenCL version is recent enough to support those builtins.
