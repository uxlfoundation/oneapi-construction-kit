// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CLC OPTIONS: -cl-std=CL3.0
__kernel void flag_local_clear_set(__global int *out_buffer,
                                   volatile __local atomic_flag *local_buffer) {
  uint gid = get_global_id(0);
  uint lid = get_local_id(0);

  atomic_flag_clear_explicit(local_buffer + lid, memory_order_relaxed,
                             memory_scope_work_item);
  out_buffer[gid] = atomic_flag_test_and_set_explicit(
      local_buffer + lid, memory_order_relaxed, memory_scope_work_item);
}
