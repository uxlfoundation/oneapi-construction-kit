// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void boscc_nested_loops3(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;
  if (id < n) {
    for (int i = 0; i < n; ++i) {
      int mul = n * id;
      int div = mul / n + id;
      int shl = div << 3;
      int wrong = mul + div + shl + i;
      for (; i < n; ++i) {
        int add = wrong + id;
        int j = 0;
        while (true) {
          ret++;
          if (wrong < n) {
            int mul2 = mul * mul;
            int div2 = mul2 / n;
            int shl2 = div2 << 3;
            ret += shl2 + add;
          }
          wrong++;
          if (id + j++ >= n) {
            break;
          }
        }
      }
    }
  }
  out[id] = ret;
}
