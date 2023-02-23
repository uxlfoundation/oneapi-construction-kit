// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void convert2(__global long2 *in, __global float2 *out) {
  size_t x = get_global_id(0);
  out[x] = convert_float2( in[x] );
}
