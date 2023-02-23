// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// This tests that the loop-computed value is handled correctly by the control
// flow conversion, namely that its value depends on the global ID despite
// having no direct dependence on it except through the control flow.
__kernel void varying_lcssa_phi(__global ushort *src, __global ushort *dst) {
  size_t x = get_global_id(0);
  unsigned short hash = 0;
  for (size_t i = 0; i < x; ++i) {
    // We explicitly cast 40499 to a uint here to avoid UB. If an
    // expression contains a ushort and ushort can be represented as int it
    // will be promoted to int in the expression. If the result of experession
    // then overflows int, you'll get UB.
    hash = hash * (uint)40499 + src[i];
  }

  if (hash & 1) {
    dst[x] = src[x];
  } else {
    dst[x] = hash;
  }
}
