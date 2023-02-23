// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void vecz_lcssa(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;
  if (id % 2 == 0) {
    int mul = n * id;
    int div = mul / n + id;
    int shl = div << 3;
    int x = mul + div + shl;
    for (int i = 0; i < n; ++i) {
      if (id <= 8) {
        for (int j = 0; j < id; ++j) {
          ret++;
          int mul2 = mul * mul;
          int div2 = mul2 / n;
          int shl2 = div2 << 3;
          ret += shl2 + x;
          if (id >= 4) {
            break;
          }
        }
      }
    }
  }
  out[id] = ret;
}
