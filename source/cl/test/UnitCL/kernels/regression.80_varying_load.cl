// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void varying_load(__global int *out, int n, __global int *meta) {
  int id = get_global_id(0);
  int ret = 0;

  if (id <= 10) {
    int sum = n;
    if (*meta == 0) {
      int mul = n * id;
      int div = mul / n + id;
      int shl = div << 3;
      mul += shl;
      sum = mul << 3;
    }

    if (id % 2 == 0) {
      sum *= *meta + n;
      ret = sum;
    }
  }
  out[id] = ret;
}
