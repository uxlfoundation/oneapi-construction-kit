// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void abs4_builtin(__global float4 *in, __global float4 *out) {
  size_t tid = get_global_id(0);

  out[tid] = fabs(in[tid]);
}
