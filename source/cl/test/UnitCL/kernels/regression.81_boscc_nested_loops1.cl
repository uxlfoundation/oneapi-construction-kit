// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void boscc_nested_loops1(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;
  if (id % 2 == 0) {
    const bool cmp = n == 5;
    int mul = n * id;
    int div = mul / n + id;
    int shl = div << 3;
    int x = shl + mul + div;
    for (int i = 0; i < n; ++i) {
      if (cmp) {
        ret += x;
      }
      if (n % 2 != 0) {
        if (n > 3) {
          for (int j = 0; j < n; ++j) {
            ret++;
            if (id == 0) {
              int mul2 = mul * mul;
              int div2 = mul2 / n;
              int shl2 = div2 << 3;
              ret += shl2;
            }
            for (int k = 0; k < n; ++k) {
              ret += x;
              if (id == 4) {
                int mul2 = mul * mul;
                int div2 = mul2 / n;
                int shl2 = div2 << 3;
                ret += shl2;
              }
            }
          }
        }
      }
    }
  }
  out[id] = ret;
}
