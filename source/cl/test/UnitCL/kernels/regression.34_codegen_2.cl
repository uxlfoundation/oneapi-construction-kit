// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void codegen_2(const __global int *in, __global int *out, int size,
                        int reps) {
  size_t gid = get_global_id(0);
  int sum = 0;

  for (size_t i = gid * reps; i < (gid + 1) * reps; i++) {
    if (i < size) sum += in[i];
  }

  out[gid] = sum;
}
