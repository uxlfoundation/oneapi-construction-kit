// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void danger_div_hoist(__global int *out, int r) {
  size_t id = get_global_id(0);

  int result = r;
  int div = ((id * 237) & 0xF);
  if (div != 0) {
    result /= div;
  }
  out[id] = result;
}
