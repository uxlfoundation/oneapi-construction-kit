// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void loop_diverge(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;

  while (1) {
    if (id < 1) {
      for (int i = 0; i < n; ++i) ret += n;
    }
    if (id < 2) {
      for (int i = 0; i < n; ++i) ret += n;
    }
    if (id < 3) {
      for (int i = 0; i < n; ++i) ret += n;
    }
    if (id < 4) {
      for (int i = 0; i < n; ++i) ret += n;
    }
    if (id < 5) {
      for (int i = 0; i < n; ++i) ret += n;
    }
    if (id < 6) {
      for (int i = 0; i < n; ++i) ret += n;
    }
    if (id < 7) {
      for (int i = 0; i < n; ++i) ret += n;
    }
    if (id < 8) {
      for (int i = 0; i < n; ++i) ret += n;
    }
    if (id < 9) {
      for (int i = 0; i < n; ++i) ret += n;
    }
    if (id < 10) {
      for (int i = 0; i < n; ++i) ret += n;
    }
    if (id < 11) {
      for (int i = 0; i < n; ++i) ret += n;
    }
    if (id < 12) {
      for (int i = 0; i < n; ++i) ret += n;
    }
    if (id < 13) {
      for (int i = 0; i < n; ++i) ret += n;
    }
    if (id < 14) {
      for (int i = 0; i < n; ++i) ret += n;
    }
    if (id < 15) {
      for (int i = 0; i < n; ++i) ret += n;
    }
    if (ret + id >= 15) {
      break;
    }
  }

  out[id] = ret + id;
}
