// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: parameters

// Reduced from OpenCL 1.2 CTS test_convert_ushort3_uchar3:
//     ./conformance_test_conversions -3 ushort_uchar
__kernel void ushort3_uchar3_codegen(__global uchar *src,
                                     __global ushort *dest) {
  size_t gid = get_global_id(0);
  uchar3 in = vload3(gid, src);
  ushort3 out = CONVERT_FUNCTION(in);
  vstore3(out, gid, dest);
}
