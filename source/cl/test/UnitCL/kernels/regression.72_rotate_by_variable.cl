// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void rotate_by_variable(__global uint *in1, __global uint *in2,
                                 __global uint *out) {
  size_t tid = get_global_id(0);

  out[tid] = rotate(in1[tid], in2[tid]);
}
