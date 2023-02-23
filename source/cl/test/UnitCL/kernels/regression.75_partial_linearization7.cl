// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void partial_linearization7(__global int *out, int n) {
  int id = get_global_id(0);
  int i = 0;

  if (n > 10) {
    if (n + id > 15) {
      i = n * 10;
      goto g;
    } else {
      goto e;
    }
  } else {
    if (n < 5) {
      goto e;
    } else {
      for (int j = 0; j < n; j++) { i++; }
      goto h;
    }
  }

e:
  if (n > 5) {
    goto g;
  } else {
    i = n * 3 / 5;
    goto h;
  }

g:
  for (int j = 0; j < n; j++) { i++; }
  goto i;

h:
  i = n + id / 3;

i:
  out[id] = i;
}
