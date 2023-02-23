// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void add3(global int3 *in1, global int3 *in2, global int3 *out) {
  size_t tid = get_global_id(0);

  int3 a = vload3(0, (global int *)&in1[tid]);
  int3 b = vload3(0, (global int *)&in2[tid]);
  int3 c = a + b;

  vstore3(c, 0, (global int *)&out[tid]);
}
