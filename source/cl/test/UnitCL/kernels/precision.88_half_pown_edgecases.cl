// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: half
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

__kernel void half_pown_edgecases(__global half* x,
                                  __global int* n,
                                  __global ushort* out) {
  size_t id = get_global_id(0);
  half in = x[id];
  const half result = pown(in, n[id]);
  out[id] = as_ushort(result);
}
