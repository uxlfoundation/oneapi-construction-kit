// Copyright (C) Codeplay Software Limited. All Rights Reserved.
//
// TODO: CA-1830, FENCE_OP is parameterised for the online case. We need to set this up
// for the offline case as well.
// DEFINITIONS: "-DFENCE_OP=mem_fence"

__kernel void memory_fence_local(__global int* in, __local int* tmp,
                                 __global int* out) {
  size_t gid = get_global_id(0);
  size_t lid = get_local_id(0);
  tmp[lid] = in[gid];

  // This fence basically doesn't affect the test, but mem_fence is so
  // toothless that it is very hard to write a test that would fail without a
  // fence but pass with.  Having it here does at least exercise compiler
  // support for a fence though.
  FENCE_OP(CLK_LOCAL_MEM_FENCE);

  out[gid] = tmp[lid];
}
