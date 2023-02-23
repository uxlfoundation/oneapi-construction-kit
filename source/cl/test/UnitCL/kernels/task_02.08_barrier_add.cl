// Copyright (C) Codeplay Software Limited. All Rights Reserved.
//
// DEFINITIONS: "-DARRAY_SIZE=8"

#if defined(KTS_DEFAULTS)
#define ARRAY_SIZE 64
#endif

__kernel void barrier_add(__global int *in1, __global int *in2,
                          __global int *out) {
  size_t tid = get_global_id(0);
  size_t lid = get_local_id(0);
  __local volatile int temp[ARRAY_SIZE];
  temp[lid] = in1[tid] + in2[tid];
  barrier(CLK_LOCAL_MEM_FENCE);

  size_t lsize = get_local_size(0);
  size_t base = get_group_id(0) * lsize;
  int num_correct = 0;
  for (unsigned i = 0; i < lsize; i++) {
    int expected = in1[base + i] + in2[base + i];
    int actual = temp[i];
    num_correct += (expected == actual) ? 1 : 0;
  }
  out[tid] = (num_correct == lsize);
}
