// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void copy3(global int3 *in, global int3 *out) {
  size_t tid = get_global_id(0);

  int3 v = vload3(0, (global int *)&in[tid]);
  vstore3(v, 0, (global int *)&out[tid]);
}
