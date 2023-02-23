// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void shuffle_copy(__global const int2 *source, __global int8 *dest) {
  int8 tmp = (int8)((int)0);
  tmp.s0 = source[get_global_id(0)].s0;
  dest[get_global_id(0)] = tmp;
}
