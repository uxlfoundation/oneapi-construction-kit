// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CL_STD: 3.0
__kernel void fetch_global_max_check_return_int(
    volatile __global atomic_int *input_buffer, __global int *output_buffer) {
  uint gid = get_global_id(0);

  output_buffer[gid] = atomic_fetch_max_explicit(
      input_buffer + gid, 0, memory_order_relaxed, memory_scope_work_item);
}
