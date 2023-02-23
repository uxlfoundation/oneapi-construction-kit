Feature additions:

* Introduce the ability to specialize builtin functions for specific targets.
  This is done using the `add_target_builtins()` CMake command and providing a
  list of source files which implement optimized versions of builtins for a
  target architecture. Targets can then access the embedded LLVM bitcode at
  runtime to make use of the optimized builtin functions.
