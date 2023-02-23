// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void codegen_1(const __global int *in0, const __global int *in1,
                        const __global int *in2, __global int *out,
                        const __global int *size, int reps) {
  size_t gid = get_global_id(0);
  int sum = 0;

  for (size_t i = gid * reps; i < (gid + 1) * reps; i++) {
    if (i < size[0]) sum += in0[i];
    if (i < size[1]) sum += in1[i];
    if (i < size[2]) sum += in2[i];
  }

  out[gid] = sum;
}
