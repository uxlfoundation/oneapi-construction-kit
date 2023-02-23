// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void convert3(__global long3 *in, __global float3 *out) {
  size_t x = get_global_id(0);
  out[x] = convert_float3( in[x] );
}
