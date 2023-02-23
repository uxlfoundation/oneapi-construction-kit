// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void cross_elem4_zero(__global float4 *in1, __global float4 *in2,
                             __global float4 *out) {
  size_t tid = get_global_id(0);
  out[tid] = cross(in1[tid], in2[tid]);
}
