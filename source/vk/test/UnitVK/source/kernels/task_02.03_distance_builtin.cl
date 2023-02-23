// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void distance_builtin(__global float *in1, __global float *in2,
                               __global float *out) {
  size_t tid = get_global_id(0);

  // There is no such version of distance: float4 distance(float4, float4)
  out[tid] = distance(in1[tid], in2[tid]);
}
