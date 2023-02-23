// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void copy_uniform_offset(__global int *in, __global int *out,
                                  const int offset) {
  size_t tid = get_global_id(0);

  out[(offset * 4) + tid] = in[tid];
}
