Feature additions:

* The `host` target now uses LLVM's `ORC`-based JIT to execute online kernels,
  rather than the older `MCJIT`.
* The `CA_HOST_TARGET_CPU` environment variable may be set to `"native"` at
  runtime to compiler and execute JIT kernels for the host system. The
  behaviour should be identical to that when the project is built with
  `-DCA_HOST_TARGET_CPU=native`.
