// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void break_loop(global int *in1, global int *in2, global int *out) {
  size_t gid = get_global_id(0);
  int i = 0;
  for (i = 0; i < 32; i++) {
    if (in1[gid] == 0) {
      break;
    }
    if (in1[gid] == -1) {
      break;
    }
  }
  out[gid] = in2[gid] + i;
}
