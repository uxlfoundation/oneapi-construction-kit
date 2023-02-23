// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void danger_div_hoist_long(__global int *out, long r) {
  size_t id = get_global_id(0);

  long result = r;
  long div = ((id * 237) & 0xF);
  if (div != 0) {
    result /= div;
  }
  out[id] = result;
}
