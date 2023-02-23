// Copyright (C) Codeplay Software Limited. All Rights Reserved.

kernel __attribute__((noinline)) void kernel_signature_noinline_after(
    global int* in, global int* out) {
  size_t gid = get_global_id(0);
  out[gid] = in[gid];
}

__attribute__((noinline)) size_t function_noinline() {
  return get_global_id(0);
}
