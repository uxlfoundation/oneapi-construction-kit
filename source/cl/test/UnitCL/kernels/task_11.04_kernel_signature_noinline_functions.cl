// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__attribute__((noinline)) size_t function_noinline_a(size_t a) { return a; }

kernel __attribute__((noinline)) void kernel_signature_noinline_functions(
    global int* in, global int* out) {
  size_t gid = function_noinline_a(get_global_id(0));
  out[gid] = in[gid];
}

__attribute__((noinline)) size_t function_noinline_ab(size_t b, int a) {
  return a;
}

__attribute__((noinline)) size_t function_noinline() {
  return get_global_id(0);
}

kernel __attribute__((noinline)) void
kernel_signature_noinline_functions_second(global int* in, global int* out) {
  size_t gid = function_noinline();
  gid = function_noinline_ab(gid, gid);
  out[gid] = in[gid];
}