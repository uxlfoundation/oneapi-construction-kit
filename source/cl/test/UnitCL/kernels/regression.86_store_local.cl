// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void __attribute__((noinline))
store(__global int *out, int gid, __local int *ret, int n) {
  *ret = n;
  out[gid] = *ret;
}

__kernel void
store_local(__global int *out, int n) {
  size_t gid = get_global_id(0);

  __local int ret;

  store(out, gid, &ret, n);
}
