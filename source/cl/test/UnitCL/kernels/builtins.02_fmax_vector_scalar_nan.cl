// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void fmax_vector_scalar_nan(__global float4 *in, uint nancode,
                                     __global float4 *out) {
  size_t tid = get_global_id(0);

  out[tid] = fmax(in[tid], nan(nancode));
}
