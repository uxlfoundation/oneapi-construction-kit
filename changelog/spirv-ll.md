Feature additions:

* `spirv-ll-tool` now outputs to stdout when given `-o -`.
* `spirv-ll` now generates calls to ComputeMux synchronization builtins for
  `OpControlBarrier` (`__mux_work_group_barrier` and `__mux_sub_group_barrier`)
  and `OpMemoryBarrier` (`__mux_mem_barrier`).
