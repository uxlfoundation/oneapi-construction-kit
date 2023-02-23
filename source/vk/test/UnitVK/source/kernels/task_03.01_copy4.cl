// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void copy4(__global int4 *in, __global int4 *out) {
  size_t tid = get_global_id(0);

  out[tid] = in[tid];
}
