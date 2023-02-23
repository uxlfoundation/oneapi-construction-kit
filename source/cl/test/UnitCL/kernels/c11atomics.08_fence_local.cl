// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// CLC OPTIONS: -cl-std=CL3.0
__kernel void fence_local(__global int* in, __global int* out,
                          __local int* tmp) {
  uint gid = get_global_id(0);
  uint lid = get_local_id(0);
  tmp[lid] = in[gid];
  atomic_work_item_fence(CLK_LOCAL_MEM_FENCE, memory_order_relaxed,
                         memory_scope_work_item);
  out[gid] = tmp[lid];
}
