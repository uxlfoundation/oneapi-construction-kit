// Copyright (C) Codeplay Software Limited. All Rights Reserved.

int identity(int x) { return x; }

__kernel void user_fn_identity(__global int *in, __global int *out) {
  size_t tid = get_global_id(0);
  out[tid] = identity(in[tid]);
}
