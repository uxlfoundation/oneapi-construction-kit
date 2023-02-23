// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void partial_linearization3(__global int *out, int n) {
  int id = get_global_id(0);
  int ret = 0;

  if (n > 10) {
    if (id % 3 == 0) {
      for (int i = 0; i < n - 1; i++) { ret++; } goto end;
    } else {
      for (int i = 0; i < n / 3; i++) { ret += 2; } goto h;
    }
  } else {
    if (id % 2 == 0) {
      for (int i = 0; i < n * 2; i++) { ret += 1; } goto h;
    } else {
      for (int i = 0; i < n + 5; i++) { ret *= 2; } goto i;
    }
  }

h:
  ret += 5;

i:
  ret *= 10;

end:
  out[id] = ret;
}
