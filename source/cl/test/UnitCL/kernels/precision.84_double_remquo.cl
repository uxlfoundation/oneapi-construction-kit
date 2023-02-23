// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: double

__kernel void double_remquo(__global int *out, double x, double y) {
  size_t i = get_global_id(0);
  int i0 = 0xdeaddead;
  remquo(x, y, &i0);
  out[i] = i0;
}
