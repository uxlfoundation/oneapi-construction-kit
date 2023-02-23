// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void partial_linearization5(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;

  if (id % 2 == 0) {
    if (id == 4) {
      goto g;
    } else {
      goto d;
    }
  } else {
    if (n % 2 == 0) {
      goto d;
    } else {
      goto e;
    }
  }

d:
  for (int i = 0; i < n; i++) { ret += i - 2; }
  goto f;

e:
  for (int i = 0; i < n + 5; i++) { ret += i + 5; }

f:
  ret *= ret % n;
  ret *= ret + 4;

g:
  out[id] = ret;
}
