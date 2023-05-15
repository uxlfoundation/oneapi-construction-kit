// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CL_STD: 3.0
__kernel void compare_exchange_strong_global_local_single_uint(
    volatile __global atomic_uint *atomic, __global uint *expected_buffer,
    __global uint *desired_buffer, int __global *bool_output_buffer,
    __local uint *expected_local_buffer) {
  int gid = get_global_id(0);
  int lid = get_local_id(0);

  expected_local_buffer[lid] = expected_buffer[gid];

  bool_output_buffer[gid] = atomic_compare_exchange_strong_explicit(
      atomic, expected_local_buffer + lid, desired_buffer[gid],
      memory_order_relaxed, memory_order_relaxed, memory_scope_work_item);

  expected_buffer[gid] = expected_local_buffer[lid];
}
