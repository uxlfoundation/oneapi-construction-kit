// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
__kernel void fetch_local_min_check_return(
    __global TYPE *input_buffer, __global TYPE *output_buffer,
    volatile __local ATOMIC_TYPE *local_buffer) {
  uint gid = get_global_id(0);
  uint lid = get_local_id(0);

  atomic_init(local_buffer + lid, input_buffer[gid]);

  output_buffer[gid] = atomic_fetch_min_explicit(
      local_buffer + lid, 0, memory_order_relaxed, memory_scope_work_item);
}
