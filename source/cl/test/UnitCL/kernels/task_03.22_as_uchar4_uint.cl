// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void as_uchar4_uint(__global uint *in, __global uchar4 *out) {
  size_t tid = get_global_id(0);
  out[tid] = as_uchar4(in[tid]);
}
