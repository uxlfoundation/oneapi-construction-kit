// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void convert(__global long *in, __global float *out) {
  size_t x = get_global_id(0);
  out[x] = convert_float( in[x] );
}
