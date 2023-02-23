// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void modf_builtin(global float *in, global float *outFrac,
                         global float *outInt) {
  size_t tid = get_global_id(0);

  outFrac[tid] = modf(in[tid], &outInt[tid]);
}
