// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
__kernel void compare_exchange_strong_global_private(
    volatile __global ATOMIC_TYPE *atomic_buffer,
    __global TYPE *expected_buffer, __global TYPE *desired_buffer,
    int __global *bool_output_buffer) {
  int gid = get_global_id(0);
  int lid = get_global_id(0);

  TYPE private_expected = expected_buffer[gid];

  bool_output_buffer[gid] = atomic_compare_exchange_strong_explicit(
      atomic_buffer + gid, &private_expected, desired_buffer[gid],
      memory_order_relaxed, memory_order_relaxed, memory_scope_work_item);

  expected_buffer[gid] = private_expected;
}
