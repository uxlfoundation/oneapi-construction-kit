// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void local_atomic(__global uint* out) {
  __local uint local_count;

  if (get_local_id(0) == 0) {
    local_count = 0;
  }

  barrier(CLK_LOCAL_MEM_FENCE);

  atomic_inc(&local_count);

  barrier(CLK_LOCAL_MEM_FENCE);

  if (get_local_id(0) == 0) {
    out[get_group_id(0)] = local_count;
  }
}
