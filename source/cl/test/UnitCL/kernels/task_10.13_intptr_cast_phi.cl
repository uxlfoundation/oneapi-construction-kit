// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void intptr_cast_phi(__global char *in, __global int *out) {
  const size_t id = get_global_id(0);
  uintptr_t intin = (uintptr_t)in + (id << 4);
  for (int x = 0; x < 4; ++x) {
    out[(id << 2) + x] = *((__global int*)intin);
    intin += 4;
  }
}
