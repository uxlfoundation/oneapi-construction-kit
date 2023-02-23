// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
__kernel void exchange_global(volatile __global ATOMIC_TYPE *in_out_buffer,
                              __global TYPE *desired_buffer,
                              __global TYPE *output_buffer) {
  uint gid = get_global_id(0);
  output_buffer[gid] =
      atomic_exchange_explicit(in_out_buffer + gid, desired_buffer[gid],
                               memory_order_relaxed, memory_scope_work_item);
}
