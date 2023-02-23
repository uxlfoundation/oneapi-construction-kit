// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void add4f(__global float4 *in1, __global float4 *in2,
                    __global float4 *out) {
  size_t tid = get_global_id(0);
  out[tid] = in1[tid] + in2[tid];
}
