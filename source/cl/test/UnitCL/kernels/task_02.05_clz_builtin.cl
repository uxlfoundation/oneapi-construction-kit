// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void clz_builtin(__global uint *in, __global uint *out) {
  size_t tid = get_global_id(0);

  out[tid] = clz(in[tid]);
}
