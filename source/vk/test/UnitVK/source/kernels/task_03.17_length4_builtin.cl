// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void length4_builtin(global float4 *in, global float *out) {
  size_t tid = get_global_id(0);

  out[tid] = length(in[tid]);
}
