// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void modf4_builtin(global float4 *in, global float4 *outFrac,
                          global float4 *outInt) {
  size_t tid = get_global_id(0);

  outFrac[tid] = modf(in[tid], &outInt[tid]);
}
