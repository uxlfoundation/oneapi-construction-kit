// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
__kernel void fetch_global_or_truth_table(volatile __global ATOMIC_TYPE *total,
                                        __global TYPE *input_buffer) {
  atomic_fetch_or_explicit(total, input_buffer[0], memory_order_relaxed,
                           memory_scope_work_item);
}
