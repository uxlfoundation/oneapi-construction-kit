Upgrade guidance:

* The `compiler::utils::SetBarrierConvergentPass` is now run when compiling
  OpenCL C kernels and not just on SPIR/SPIRV modules.
* The `compiler::utils::SetBarrierConvergentPass` has been renamed
  `SetConvergentAttrPass`.
* The `compiler::utils::SetConvergentAttrPass` now sets the `convergent`
  attribute to any function not known to the `BuiltinInfo` to be
  non-convergent. This property is bubbled up throuh the call-graph so that
  callers of convergent functions are themselves marked convergent.
* The declarations of `sub_group_*` OpenCL builtins are now marked convergent.
