// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void normalize4_builtin(global float4 *in, global float4 *out) {
  size_t tid = get_global_id(0);

  out[tid] = normalize(in[tid]);
}
