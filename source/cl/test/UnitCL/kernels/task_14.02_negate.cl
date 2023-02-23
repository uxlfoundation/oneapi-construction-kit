// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// This kernel is meant to test scalarization and vectorization of FNeg.
__kernel void negate(__global float4 *in, __global float4 *out) {
  size_t x = get_global_id(0);
  out[x] = - in[x];
}
