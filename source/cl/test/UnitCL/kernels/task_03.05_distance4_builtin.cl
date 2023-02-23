// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void distance4_builtin(__global float4 *in1, __global float4 *in2,
                                __global float *out) {
  size_t tid = get_global_id(0);

  // There is no such version of distance: float4 distance(float4, float4)
  out[tid] = distance(in1[tid], in2[tid]);
}
