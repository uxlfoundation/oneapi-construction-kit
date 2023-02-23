// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void clz4_builtin(__global uint4 *in, __global uint4 *out) {
  size_t tid = get_global_id(0);

  out[tid] = clz(in[tid]);
}
