// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void clamp_builtin(__global float *in, __global float *out, float low,
                            float high) {
  size_t tid = get_global_id(0);

  // The vector variant should be: floatn clamp(floatn, float, float)
  out[tid] = clamp(in[tid], low, high);
}
