// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CL_STD: 3.0
__kernel void fetch_global_or_truth_table_uint(volatile __global atomic_uint *total,
                                        __global uint *input_buffer) {
  atomic_fetch_or_explicit(total, input_buffer[0], memory_order_relaxed,
                           memory_scope_work_item);
}
