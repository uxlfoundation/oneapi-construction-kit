// Copyright (C) Codeplay Software Limited. All Rights Reserved.

int foo(int x, int y) { return x * (y - 1); }

kernel void user_fn_two_contexts(global int *out, global int *in,
                                 global int *in2, int alpha) {
  size_t tid = get_global_id(0);
  int src1 = in[tid];
  int src2 = in2[tid];
  int res1 = foo(src1, src2);   // varying, varying params
  int res2 = foo(alpha, src2);  // uniform, varying params
  out[tid] = res1 + res2;
}
