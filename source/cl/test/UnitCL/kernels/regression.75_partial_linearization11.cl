// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void partial_linearization11(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;
  int i = 0;

  while (1) {
    while (1) {
      if (n > 5) {
        for (int i = 0; i < n * 2; i++) ret++;
        if (n == 6) {
          goto i;
        }
      } else {
        if (ret + id >= n) {
          ret /= n * n + ret;
          if (ret <= 10) {
            goto k;
          } else {
            goto h;
          }
        }
      }
      ret *= n;
      if (i++ > 2) {
        goto j;
      }

      h:
      ret++;
    }

    j:
      ret += n * 2 + 20;
      goto l;

    k:
      ret *= n;
      goto l;

    l:
      if (i > 3) {
        ret++;
        goto m;
      }
  }

m:
  for (int i = 0; i < n / 4; i++) ret++;
  goto n;

i:
  ret /= n;

n:
  out[id] = ret;
}
