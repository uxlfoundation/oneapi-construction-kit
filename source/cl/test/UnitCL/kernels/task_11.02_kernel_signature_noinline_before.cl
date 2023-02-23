// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__attribute__((noinline)) size_t function_noinline() {
  return get_global_id(0);
}

kernel __attribute__((noinline)) void kernel_signature_noinline_before(
    global int* in, global int* out) {
  size_t gid = function_noinline();
  out[gid] = in[gid];
}
