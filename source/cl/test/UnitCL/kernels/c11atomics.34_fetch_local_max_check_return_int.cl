// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CL_STD: 3.0
__kernel void fetch_local_max_check_return_int(
    __global int *input_buffer, __global int *output_buffer,
    volatile __local atomic_int *local_buffer) {
  uint gid = get_global_id(0);
  uint lid = get_local_id(0);

  atomic_init(local_buffer + lid, input_buffer[gid]);

  output_buffer[gid] = atomic_fetch_max_explicit(
      local_buffer + lid, 0, memory_order_relaxed, memory_scope_work_item);
}
