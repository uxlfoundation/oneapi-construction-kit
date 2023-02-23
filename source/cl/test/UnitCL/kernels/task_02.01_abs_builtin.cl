// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void abs_builtin(__global int *in, __global uint *out) {
  size_t tid = get_global_id(0);

  out[tid] = abs(in[tid]);
}
