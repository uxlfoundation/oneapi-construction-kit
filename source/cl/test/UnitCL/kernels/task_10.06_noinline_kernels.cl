// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel __attribute__((noinline)) void kernel2(global int* in, global int* out) {
  size_t gid = get_global_id(0);
  out[gid] = in[gid];
}

kernel __attribute__((noinline)) void noinline_kernels(global int* in,
                                                       global int* out) {
  kernel2(in, out);
}
