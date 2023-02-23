// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void sycl_barrier(__global int* ptr, __local int* tile) {
  int idx = get_global_id(0);
  int pos = idx & 1;
  int opp = pos ^ 1;

  tile[pos] = ptr[idx];

  barrier(CLK_LOCAL_MEM_FENCE);

  ptr[idx] = tile[opp];
}
