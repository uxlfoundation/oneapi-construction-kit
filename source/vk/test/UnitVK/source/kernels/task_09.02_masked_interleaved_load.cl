// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void masked_interleaved_load(__global int *in, __global int *out) {
  size_t tid = get_global_id(0);
  int tmp;
  if (12 == tid) {
    tmp = in[tid * 2];
  } else {
    tmp = 0;
  }
  out[tid] = tmp;
}
