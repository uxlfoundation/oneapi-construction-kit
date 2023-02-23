// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void addf(__global float *in1, __global float *in2,
                   __global float *out) {
  size_t tid = get_global_id(0);
  out[tid] = in1[tid] + in2[tid];
}
