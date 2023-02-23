// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// The purpose of this test is to make sure the uniform value analysis considers
// the condition `i == 4` as being varying since its value can be modified in a
// divergent block.
__kernel void partial_linearization_varying_uniform_condition(__global int *out, int n) {
  int x = get_global_id(0);
  int i = n;
  int ret = 0;

  if (x == 2) {
    for (int j = 0; j <= n + 1; ++j) { i += n; }
  }

  if (i == 4) {
    for (int j = 0; j <= n; ++j) { ret += j; }
  }
  out[x] = ret;
}
