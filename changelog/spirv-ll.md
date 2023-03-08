Feature additions:

* `spirv-ll-tool` now outputs to stdout when given `-o -`.
* `spirv-ll` now generates calls to ComputeMux synchronization builtins for
  `OpControlBarrier` (`__mux_work_group_barrier` and `__mux_sub_group_barrier`)
  and `OpMemoryBarrier` (`__mux_mem_barrier`).
* `spirv-ll` now supports the `SPV_KHR_expect_assume` extension.
* `spirv-ll` now supports the `GenericPointer` storage class, targeting address
  space 4.

Upgrade guidance:

* The `modules/spirv-ll` module has been moved to `modules/compiler/spirv-ll`.
* The external `SPIRV-Headers` has been bumped to `sdk-1.3.239.0`.

Bug fixes:

* A bug was fixed in handling function calls to function forward references,
  which was previously generating invalid LLVM IR.
