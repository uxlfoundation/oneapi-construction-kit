# BuiltinInfo refactoring:

Feature additions:
* LLVM Intrinsics are now handled directly by the main BuiltinInfo object,
  instead of the language/target-dependent implementations, ensuring consistent
  scalarization and vectorization/widening.
