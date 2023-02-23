// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// It is a variant of regression.23_shuffle_copy, which failed on some configs
// with BOSCC and disabled optimizations. This version forces retention of the
// allocas.
__kernel void alloca_boscc_confuser(__global int *source, __global int *dest) {
  if (get_global_id(0) == 0) return;

  int3 tmp1;
  int3 tmp2;
  int3 *tmp = get_global_id(1) & 1 ? &tmp1 : &tmp2;

  *tmp = (int3)(10);
  (*tmp).s2 = source[0];
  vstore3(*tmp, 0, dest);

  tmp1 = (int3)(11);
  tmp1.s0 = source[1];
  vstore3(tmp1, 1, dest);

  tmp2 = (int3)(12);
  tmp2.s1 = source[2];
  vstore3(tmp2, 2, dest);
}
