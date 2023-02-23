// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
__kernel void fetch_global_add_check_return(
    volatile __global ATOMIC_TYPE *input_buffer, __global TYPE *output_buffer) {
  uint gid = get_global_id(0);

  output_buffer[gid] = atomic_fetch_add_explicit(
      input_buffer + gid, 0, memory_order_relaxed, memory_scope_work_item);
}
