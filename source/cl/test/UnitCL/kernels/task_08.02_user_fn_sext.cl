// Copyright (C) Codeplay Software Limited. All Rights Reserved.

short bar(short x) { return -x; }

kernel void user_fn_sext(global int *out, global short *in) {
  size_t tid = get_global_id(0);
  int baz = bar(in[tid]);
  out[tid] = baz;
}
