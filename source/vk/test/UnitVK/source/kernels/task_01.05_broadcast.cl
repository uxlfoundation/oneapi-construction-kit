// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void broadcast(__global int *out) {
  size_t tid = get_global_id(0);
  out[tid] = tid;
}
