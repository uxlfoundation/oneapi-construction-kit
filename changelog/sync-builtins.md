Upgrade guidance:

* Added the ``__mux_mem_barrier``, ``__mux_work_group_barrier``, and
  ``__mux_sub_group_barrier`` builtins. They replace the older
  ``__mux_global_barrier``, ``__mux_shared_local_barrier``, and
  ``__mux_full_barrier`` builtins, which have been removed. See the
  documentation for how the new builtins should be used.
