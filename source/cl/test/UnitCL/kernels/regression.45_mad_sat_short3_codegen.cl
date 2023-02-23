// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void mad_sat_short3_codegen(__global short *srcA, __global short *srcB,
                                     __global short *srcC,
                                     __global short *dst) {
  int gid = get_global_id(0);
  short3 sA = vload3(gid, srcA);
  short3 sB = vload3(gid, srcB);
  short3 sC = vload3(gid, srcC);
  short3 result = mad_sat(sA, sB, sC);
  vstore3(result, gid, dst);
}
