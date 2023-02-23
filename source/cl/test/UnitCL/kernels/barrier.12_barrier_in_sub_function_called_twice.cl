// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__attribute__((noinline)) void Barrier_Function() {
  barrier(CLK_LOCAL_MEM_FENCE);
}

__attribute__((noinline)) void Func() {
  Barrier_Function();
  Barrier_Function();
}

__kernel void barrier_in_sub_function_called_twice(__global int* ptr,
                                                   __global int* ptr2) {
  __local int tile[2];

  int idx = get_global_id(0);
  int pos = idx & 1;
  int opp = pos ^ 1;

  Func();  // Double barrier

  tile[pos] = ptr[idx];

  Func();  // Double barrier

  ptr[idx] = tile[opp];
  ptr2[idx] = tile[opp];
}
