// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CL_STD: 3.0
__kernel void fetch_global_add_check_return_uint(
    volatile __global atomic_uint *input_buffer, __global uint *output_buffer) {
  uint gid = get_global_id(0);

  output_buffer[gid] = atomic_fetch_add_explicit(
      input_buffer + gid, 0, memory_order_relaxed, memory_scope_work_item);
}
