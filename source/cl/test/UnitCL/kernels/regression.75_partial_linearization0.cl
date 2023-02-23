// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void partial_linearization0(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;

  if (id % 5 == 0) {
    for (int i = 0; i < n * 2; i++) ret++;
  } else {
    for (int i = 0; i < n / 4; i++) ret++;
  }

  if (n > 10) {
    if (id % 2 == 0) {
      for (int i = 0; i < n + 10; i++) ret++;
    } else {
      for (int i = 0; i < n + 10; i++) ret *= 2;
    }
    ret += id * 10;
  } else {
    if (id % 2 == 0) {
      for (int i = 0; i < n + 8; i++) ret++;
    } else {
      for (int i = 0; i < n + 8; i++) ret *= 2;
    }
    ret += id / 2;
  }
  out[id] = ret;
}
