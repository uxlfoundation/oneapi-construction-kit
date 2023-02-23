// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
__kernel void compare_exchange_strong_local_global(
    __global TYPE *inout, __global TYPE *expected_buffer,
    __global TYPE *desired_buffer, __global int *bool_output_buffer,
    __local volatile ATOMIC_TYPE *local_atomic_buffer) {
  int gid = get_global_id(0);
  int lid = get_local_id(0);

  atomic_init(local_atomic_buffer + lid, inout[gid]);

  bool_output_buffer[gid] = atomic_compare_exchange_strong_explicit(
      local_atomic_buffer + lid, expected_buffer + gid, desired_buffer[gid],
      memory_order_relaxed, memory_order_relaxed, memory_scope_work_item);

  inout[gid] = atomic_load_explicit(
      local_atomic_buffer + lid, memory_order_relaxed, memory_scope_work_item);
}
