// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void alloca(__global int *out) {
  size_t tid = get_global_id(0);
  int offset = get_global_offset(0);
  int temp[1];
  temp[0] &= offset;  // zero temp[0], without the compiler knowing
  temp[0] |= (int)tid;
  out[tid] = temp[0];
}
