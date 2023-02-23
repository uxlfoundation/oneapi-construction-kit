// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void shuffle_copy(__global int *source, __global int *dest) {
  if (get_global_id(0) == 0) return;

  int3 tmp;

  tmp = (int3)(10);
  tmp.s2 = source[0];
  vstore3(tmp, 0, dest);

  tmp = (int3)(11);
  tmp.s0 = source[1];
  vstore3(tmp, 1, dest);

  tmp = (int3)(12);
  tmp.s1 = source[2];
  vstore3(tmp, 2, dest);
}
