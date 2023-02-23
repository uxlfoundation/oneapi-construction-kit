// Copyright (C) Codeplay Software Limited. All Rights Reserved.

__kernel void mad_sat_ushort3_codegen(__global ushort *srcA,
                                      __global ushort *srcB,
                                      __global ushort *srcC,
                                      __global ushort *dst) {
  int gid = get_global_id(0);
  ushort3 sA = vload3(gid, srcA);
  ushort3 sB = vload3(gid, srcB);
  ushort3 sC = vload3(gid, srcC);
  ushort3 result = mad_sat(sA, sB, sC);
  vstore3(result, gid, dst);
}
