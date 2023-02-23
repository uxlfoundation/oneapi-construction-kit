// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void partial_linearization8(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;

  int x = id / n;
  int y = id % n;
  int i = 0;
  for (;;) {
    if (i + id > n) goto e;
    if (x + y > n) goto f;
    y++;
    x++;
    i++;
  }

goto g;

e:
  i *= 2 + n;
  goto g;

f:
  i /= i + n;

g:
  out[id] = x + y + i;
}
