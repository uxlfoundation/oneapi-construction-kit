// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void vector_loop(__global int *in, __global int *out) {
  size_t tid = get_global_id(0);

  if (tid != 0) return;

  int4 i = (int4)0;
  for (; (i < (int4)get_global_size(0)).s0; ++i) {
    out[i.s0] = in[i.s0];
    out[i.s1] = in[i.s1];
    out[i.s2] = in[i.s2];
    out[i.s3] = in[i.s3];
  }
}
