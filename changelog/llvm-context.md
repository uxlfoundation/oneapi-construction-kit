Upgrade guidance:

* Each `compiler::BaseTarget` now owns its own `LLVMContext`, rather than the
  (shared) `compiler::BaseContext` owning it.
