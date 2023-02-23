// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void add2(__global int2 *in1, __global int2 *in2, __global int2 *out) {
  size_t tid = get_global_id(0);

  out[tid] = in1[tid] + in2[tid];
}
