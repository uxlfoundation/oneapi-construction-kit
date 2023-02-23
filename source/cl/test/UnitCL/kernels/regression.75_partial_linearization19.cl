// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void partial_linearization19(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;
  int i = 0;

  while (1) {
    if (n > 5) {
      if (n == 6) {
        goto d;
      } else {
        goto e;
      }
    }
    if (++i + id > 3) {
      break;
    }
  }

  if (n == 3) {
    goto h;
  } else {
    goto i;
  }

d:
  for (int i = 0; i < n + 5; i++) ret += 2;
  goto i;

e:
  for (int i = 1; i < n * 2; i++) ret += i;
  goto h;

i:
  for (int i = 0; i < n + 5; i++) ret++;
  goto j;

h:
  for (int i = 0; i < n; i++) ret++;
  goto j;

j:
  out[id] = ret;
}
