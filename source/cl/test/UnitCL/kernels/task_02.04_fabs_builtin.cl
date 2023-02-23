// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void fabs_builtin(__global float *in, __global float *out) {
  size_t tid = get_global_id(0);

  out[tid] = fabs(in[tid]);
}
