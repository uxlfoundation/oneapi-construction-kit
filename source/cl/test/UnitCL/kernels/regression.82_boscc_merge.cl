// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void boscc_merge(__global int *out, __global int *in, int n, int m) {
  int id = get_global_id(0);
  int ret = 0;
  if (id % 2 == 0) {
    __global int *x = &in[n];
    if (n != 0) {
      __global int *y = &x[id - 1];
      int base = 0;
      if (m == 0) {
        base = *y;
      } else {
        if (id % 4 == 0) {
          base = y[m];
        }
      }
      ret = base;
    }
    ret += x[n + m];
  }
  out[id] = ret;
}
