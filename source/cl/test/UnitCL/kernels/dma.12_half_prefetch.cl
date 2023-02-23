// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half; parameters;
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef TYPE
#define TYPE half
#endif

__kernel void half_prefetch(__global TYPE *A, __global TYPE *B,
                            __global TYPE *C) {
  size_t gid = get_global_id(0);
  size_t group = get_group_id(0);
  size_t size = get_local_size(0);

  // Prefetch the input data.
  prefetch(&A[group * size], size);
  prefetch(&B[group * size], size);

  C[gid] = fmax(A[gid], B[gid]);
}
