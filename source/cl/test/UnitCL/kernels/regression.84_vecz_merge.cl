// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void vecz_merge(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;

  while (1) {
    if (n > 0 && n < 5) {
      goto f;
    }
    while (1) {
      if (n <= 2) {
        ret = 5;
        goto f;
      } else {
        if (ret + id >= n) {
          ret = id;
          goto d;
        }
      }
      if (n & 1) {
        ret = 1;
        goto f;
      }

d:
      if (n > 3) {
        ret = n;
        goto e;
      }
    }

e:
    if (n & 1) {
      ret = n + 2;
      goto f;
    }
  }

f:
  out[id] = ret;
}
