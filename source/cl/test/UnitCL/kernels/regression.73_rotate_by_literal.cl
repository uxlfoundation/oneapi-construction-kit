// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void rotate_by_literal(__global uint *in, __global uint *out) {
  out[0] = rotate(in[0], 0u);
  out[1] = rotate(in[1], 32u);
  out[2] = rotate(in[2], 4u);
  out[3] = rotate(in[3], 7u);
}
