// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void as_ushort2_uchar4(__global uchar4 *in, __global ushort2 *out) {
  size_t tid = get_global_id(0);
  out[tid] = as_ushort2(in[tid]);
}
