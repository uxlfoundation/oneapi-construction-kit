// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CLC OPTIONS: -cl-std=CL3.0
__kernel void fence_global(__global int* in, __global int* out,
                           __global int* tmp) {
  uint gid = get_global_id(0);
  tmp[gid] = in[gid];
  atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_relaxed,
                         memory_scope_work_item);
  out[gid] = tmp[gid];
}
