#BenchCL #

A micro-benchmark framework for testing OpenCL.

## TODO ##

Benchmarks that need to be written against the API;

*~~compile& link ~~*~~kernel creation ~~*individual queue commands* first vs
        second run of kernels* eager vs lazy command
        groups(flush on each command, flush on N commands) *
    ~~single vs multithreaded performance enqueuing on queue ~~*create buffer
          ->map->write vs create buffer copying host ptr* create buffer
      using host ptr vs create buffer

      Benchmarks that need to be written against the kernel library;

* scalar vs vector for the kernel library
* identical vector component values vs differing
* fast-math vs normal math
* native_* vs normal math
* half_* vs normal math
* non-struct vs struct kernel parameters
* wide vs narrow global work sizes
