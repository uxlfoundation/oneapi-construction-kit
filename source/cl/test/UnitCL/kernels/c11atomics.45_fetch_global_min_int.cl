// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CL_STD: 3.0
__kernel void fetch_global_min_int(volatile __global atomic_int *total,
                               __global int *input_buffer) {
  uint gid = get_global_id(0);

  atomic_fetch_min_explicit(total, input_buffer[gid], memory_order_relaxed,
                            memory_scope_work_item);
}
