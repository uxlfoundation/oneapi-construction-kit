Upgrade guidance:

* ComputeMux runtimes now handle multiple compiler-generated variants of a
  given kernel, selecting between them at runtime.
* `mux_kernel_s::sub_group_size` has been removed.
* A new mux entry point has been introduced: `muxQueryMaxNumSubGroups`.
