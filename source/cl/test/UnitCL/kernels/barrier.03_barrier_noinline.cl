// Copyright (C) Codeplay Software Limited. All Rights Reserved.

void __attribute__((overloadable, noinline)) barrier(cl_mem_fence_flags flags);

void Barrier_Function() { barrier(CLK_LOCAL_MEM_FENCE); }

__kernel void barrier_noinline(__global int* ptr, __global int* ptr2) {
  __local int tile[2];

  int idx = get_global_id(0);
  int pos = idx & 1;
  int opp = pos ^ 1;

  Barrier_Function();  // Double barrier

  tile[pos] = ptr[idx];

  Barrier_Function();  // Double barrier

  ptr[idx] = tile[opp];
  ptr2[idx] = tile[opp];
}
