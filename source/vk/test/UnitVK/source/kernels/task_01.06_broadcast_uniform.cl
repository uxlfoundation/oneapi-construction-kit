// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void broadcast_uniform(__global int *out, int foo) {
  size_t tid = get_global_id(0);
  out[tid] = foo + 1;
}
