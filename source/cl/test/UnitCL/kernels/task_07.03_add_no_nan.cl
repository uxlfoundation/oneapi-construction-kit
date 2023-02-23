// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void add_no_nan(__global float *in1, __global float *in2,
                         __global float *out) {
  size_t tid = get_global_id(0);
  float a = in1[tid];
  float b = in2[tid];
  int exclude = isnan(a) | isnan(b);
  if (!exclude) {
    out[tid] = a + b;
  } else {
    out[tid] = 0.0f;
  }
}
