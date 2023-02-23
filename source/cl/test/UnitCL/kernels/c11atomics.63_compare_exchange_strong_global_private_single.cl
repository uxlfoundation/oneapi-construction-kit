// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
__kernel void compare_exchange_strong_global_private_single(
    volatile __global ATOMIC_TYPE *atomic, __global TYPE *expected_buffer,
    __global TYPE *desired_buffer, int __global *bool_output_buffer) {
  int gid = get_global_id(0);

  TYPE expected_private = expected_buffer[gid];

  bool_output_buffer[gid] = atomic_compare_exchange_strong_explicit(
      atomic, &expected_private, desired_buffer[gid],
      memory_order_relaxed, memory_order_relaxed, memory_scope_work_item);

  expected_buffer[gid] = expected_private;
}
