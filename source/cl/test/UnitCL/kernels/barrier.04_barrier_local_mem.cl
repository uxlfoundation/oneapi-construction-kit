// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// DEFINITIONS: "-DREAD_LOCAL_SIZE=16";"-DGLOBAL_ID=1";"-DREAD_LOCAL_ID=1"

void __attribute__((overloadable))
barrier(cl_mem_fence_flags flags);

void Barrier_Function() { barrier(CLK_LOCAL_MEM_FENCE); }

__kernel void barrier_local_mem(__global const int* input,
                                __global int* output) {
  __local int cache[READ_LOCAL_SIZE];

  int global_id = get_global_id(0);
  int local_id = get_local_id(0);

  Barrier_Function();  // Double barrier

  cache[local_id] = input[global_id];

  Barrier_Function();  // Double barrier

  if (global_id == GLOBAL_ID) {
    output[0] = cache[READ_LOCAL_ID];
  }
}
