// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void partial_linearization18(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;
  int i = 0;

  while (1) {
    if (n > 5) {
      if (id + i % 2 == 0) {
        goto e;
      } else {
        goto f;
      }
    }
    if (++i + id > 3) {
      goto g;
    }
  }

f:
  for (int i = 0; i < n + 5; i++) ret += 2;
  goto g;

g:
  for (int i = 1; i < n * 2; i++) ret -= i;
  goto h;

e:
  for (int i = 0; i < n + 5; i++) ret++;
  goto i;

h:
  if (n > 3) {
i:
    ret++;
  } else {
    ret *= 3;
  }

  out[id] = ret;
}
