// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
__kernel void store_global(__global TYPE *input_buffer,
                           volatile __global ATOMIC_TYPE *output_buffer) {
  uint gid = get_global_id(0);
  atomic_store_explicit(output_buffer + gid, input_buffer[gid],
                        memory_order_relaxed, memory_scope_work_item);
}
