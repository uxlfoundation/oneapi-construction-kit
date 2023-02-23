// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void shuffle_copy(__global float16* source, __global float16* dest) {
  float16 tmp = 0;
  tmp.S8e42D0Ab = source[1].s858B6A89;
  dest[1] = tmp;
}
