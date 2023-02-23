// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void noop(__global int *dst) {
  size_t gid = get_global_id(0);
  dst[gid] = 0;

  goto foo;

  foo: {
    // All work items reach here.
    dst[gid] += 1;
  }
}
