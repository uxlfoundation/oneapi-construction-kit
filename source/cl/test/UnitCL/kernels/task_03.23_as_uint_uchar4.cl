// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void as_uint_uchar4(__global uchar4 *in, __global uint *out) {
  size_t tid = get_global_id(0);
  out[tid] = as_uint(in[tid]);
}
