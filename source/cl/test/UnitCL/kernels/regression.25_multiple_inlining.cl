// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel void foo1(global int* in, global int* out) {
  size_t gid = get_global_id(0);
  out[gid] = in[gid];
}

kernel void foo2(global int* in, global int* out) { foo1(in, out); }

kernel void foo3(global int* in, global int* out) { foo2(in, out); }

kernel void foo4(global int* in, global int* out) { foo3(in, out); }

kernel void foo5(global int* in, global int* out) { foo4(in, out); }

kernel void foo6(global int* in, global int* out) { foo5(in, out); }

kernel void multiple_inlining(global int* in, global int* out) {
  foo6(in, out);
}
