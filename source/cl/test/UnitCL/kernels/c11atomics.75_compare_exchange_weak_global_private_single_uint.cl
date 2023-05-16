// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CL_STD: 3.0
__kernel void compare_exchange_weak_global_private_single_uint(
    volatile __global atomic_uint *atomic, __global uint *expected_buffer,
    __global uint *desired_buffer, int __global *bool_output_buffer) {
  int gid = get_global_id(0);

  uint expected_private = expected_buffer[gid];

  bool_output_buffer[gid] = atomic_compare_exchange_weak_explicit(
      atomic, &expected_private, desired_buffer[gid],
      memory_order_relaxed, memory_order_relaxed, memory_scope_work_item);

  expected_buffer[gid] = expected_private;
}
