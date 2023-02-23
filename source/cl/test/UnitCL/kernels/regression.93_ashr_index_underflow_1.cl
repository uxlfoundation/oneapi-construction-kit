// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void ashr_index_underflow_1(__global const int2* in,
                                     __global int* out) {
  // note this has to be "int x" (as opposed to size_t)
  // because the bug is caused only by a signed shift.
  int x = get_global_id(0);
  out[x] = in[x >> 1].x;
}
