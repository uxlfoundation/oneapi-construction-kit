// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// DEFINITIONS: -DLOCAL_X=1

kernel void another_kernel(global int* src, global int* dst){
  __local int localBuff[LOCAL_X];

  int lid = get_local_id(0);
  int gid = get_global_id(0);
  int index = (gid * LOCAL_X) + lid;

  localBuff[lid] = src[index];

  barrier(CLK_LOCAL_MEM_FENCE);

  int result = 0;

  result = localBuff[lid];
  dst[index] = result;

}

kernel void multiple_local_memory_kernels(global int* src, global int* dst){
  __local int localBuff[LOCAL_X];

  int lid = get_local_id(0);
  int gid = get_global_id(0);
  int index = (gid * LOCAL_X) + lid;

  localBuff[lid] = src[index];

  barrier(CLK_LOCAL_MEM_FENCE);

  int result = 0;

  result = localBuff[lid];
  dst[index] = result;
}
