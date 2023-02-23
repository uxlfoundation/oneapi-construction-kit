// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void convert4(__global long4 *in, __global float4 *out) {
  size_t x = get_global_id(0);
  out[x] = convert_float4( in[x] );
}
