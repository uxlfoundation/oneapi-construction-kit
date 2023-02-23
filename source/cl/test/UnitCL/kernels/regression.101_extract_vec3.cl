// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void extract_vec3(__global int3 *src,
                           __global int *dstX,
                           __global int *dstY,
                           __global int *dstZ) {
  size_t tid = get_global_id(0);

  // For whatever reason, the `.xyz` prevents Clang from converting the load
  // from a load <3 x i32> to a load <4 x i32>
  int3 v = src[tid].xyz;
  dstX[tid] = v.x;
  dstY[tid] = v.y;
  dstZ[tid] = v.z;
}
