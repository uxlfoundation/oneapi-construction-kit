// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
__kernel void compare_exchange_strong_global_global(
    volatile __global ATOMIC_TYPE *atomic_buffer,
    __global TYPE *expected_buffer, __global TYPE *desired_buffer,
    int __global *bool_output_buffer) {
  int gid = get_global_id(0);
  bool_output_buffer[gid] = atomic_compare_exchange_strong_explicit(
      atomic_buffer + gid, expected_buffer + gid, desired_buffer[gid],
      memory_order_relaxed, memory_order_relaxed, memory_scope_work_item);
}
