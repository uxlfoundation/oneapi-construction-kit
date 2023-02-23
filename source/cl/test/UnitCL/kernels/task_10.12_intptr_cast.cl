// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void intptr_cast(__global char *in, __global int *out) {
  const size_t id = get_global_id(0);
  uintptr_t intin = (uintptr_t)in + (id << 2);
  out[id] = *((__global int*)intin);
}
