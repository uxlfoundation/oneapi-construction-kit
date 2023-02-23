// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void dot_builtin(__global float *in1, __global float *in2,
                          __global float *out) {
  size_t tid = get_global_id(0);

  // There is no such version of dot: float4 dot(float4, float4)
  out[tid] = dot(in1[tid], in2[tid]);
}
