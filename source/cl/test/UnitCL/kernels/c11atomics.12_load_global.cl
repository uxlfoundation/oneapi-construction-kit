// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
__kernel void load_global(volatile __global ATOMIC_TYPE *input_buffer,
                          __global TYPE *output_buffer) {
  uint gid = get_global_id(0);
  output_buffer[gid] = atomic_load_explicit(
      input_buffer + gid, memory_order_relaxed, memory_scope_work_item);
}
