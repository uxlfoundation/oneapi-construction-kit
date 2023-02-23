// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void divergent_memfence(int a, global int *out) {
  size_t gid = get_global_id(0);
  if (!a) {
    mem_fence(CLK_GLOBAL_MEM_FENCE);
  }
  out[gid] = gid;
}
