// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
__kernel void compare_exchange_weak_local_private_single(
    volatile __global TYPE *inout_buffer, __global TYPE *expected_buffer,
    __global TYPE *desired_buffer, int __global *bool_output_buffer,
    __local volatile ATOMIC_TYPE *local_atomic) {
  int gid = get_global_id(0);
  int lid = get_local_id(0);
  int wgid = get_group_id(0);

  TYPE expected_private = expected_buffer[gid];

  if (0 == lid) {
    atomic_init(local_atomic, inout_buffer[wgid]);
  }
  work_group_barrier(CLK_LOCAL_MEM_FENCE);

  bool_output_buffer[gid] = atomic_compare_exchange_weak_explicit(
      local_atomic, &expected_private, desired_buffer[gid],
      memory_order_relaxed, memory_order_relaxed, memory_scope_work_item);

  work_group_barrier(CLK_LOCAL_MEM_FENCE);

  if (0 == lid) {
    inout_buffer[wgid] = atomic_load_explicit(local_atomic, memory_order_relaxed,
                                             memory_scope_work_item);
  }

  expected_buffer[gid] = expected_private;
}
