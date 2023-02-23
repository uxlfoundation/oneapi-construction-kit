// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: parameters

// Reduced from OpenCL 1.2 CTS test_convert_short3_uchar3:
//     ./conformance_test_conversions -3 short_uchar
__kernel void short3_uchar3_codegen(__global uchar *src, __global short *dest) {
  size_t gid = get_global_id(0);
  uchar3 in = vload3(gid, src);
  short3 out = CONVERT_FUNCTION(in);
  vstore3(out, gid, dest);
}
