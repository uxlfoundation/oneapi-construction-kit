// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void masked_interleaved_store(__global int *in, __global int *out) {
  size_t tid = get_global_id(0);
  if (12 == tid) {
    out[tid * 2] = in[tid];
  } else {
    out[tid * 2] = 0;
  }
}
