// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void scalar_masked_load(__global int *in, __global int *out,
                                 uint target) {
  size_t tid = get_global_id(0);
  int result;
  if (tid == target) {
    result = *in * 2;
  } else {
    result = 0;
  }
  out[tid] = result;
}
