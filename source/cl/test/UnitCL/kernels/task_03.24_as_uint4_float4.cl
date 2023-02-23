// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void as_uint4_float4(__global float4 *in, __global uint4 *out) {
  size_t tid = get_global_id(0);
  out[tid] = as_uint4(in[tid]);
}
