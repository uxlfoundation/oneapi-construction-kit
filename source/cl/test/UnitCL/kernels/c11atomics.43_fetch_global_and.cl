// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
__kernel void fetch_global_and(volatile __global ATOMIC_TYPE *total,
                               __global TYPE *input_buffer) {
  uint gid = get_global_id(0);

  atomic_fetch_and_explicit(total, input_buffer[gid], memory_order_relaxed,
                            memory_scope_work_item);
}
