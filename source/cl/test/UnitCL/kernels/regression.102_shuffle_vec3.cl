// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void shuffle_vec3(__global int3 *src, __global int3 *dst) {
  size_t tid = get_global_id(0);
  dst[tid] = src[tid].yzx;
}
