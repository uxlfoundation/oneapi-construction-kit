// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void s2v_int(__global int *in, __global int4 *out) {
  size_t tid = get_global_id(0);

  out[tid] = in[tid];
}
