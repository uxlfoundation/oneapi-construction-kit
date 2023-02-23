// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void length_builtin(__global float *in, __global float *out) {
  size_t tid = get_global_id(0);

  // There is no such version of length: float4 length(float4)
  out[tid] = length(in[tid]);
}
