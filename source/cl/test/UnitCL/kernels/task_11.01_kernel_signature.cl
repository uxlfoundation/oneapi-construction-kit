// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel __attribute__((noinline)) void kernel_signature_2(global int* out) {
  size_t gid = get_global_id(0);
  out[gid] = (int)gid;
}

kernel __attribute__((noinline)) void kernel_signature(global int* in,
                                                       global int* out) {
  size_t gid = get_global_id(0);
  out[gid] = in[gid];
}
