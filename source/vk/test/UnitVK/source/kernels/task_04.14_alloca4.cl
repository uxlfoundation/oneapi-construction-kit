// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void alloca4(__global int4 *out) {
  size_t tid = get_global_id(0);
  int4 offset = (int4)get_global_offset(0);
  int4 temp[1];
  temp[0] &= offset;  // zero temp[0], without the compiler knowing
  temp[0] |= (int4)tid;
  out[tid] = temp[0];
}
