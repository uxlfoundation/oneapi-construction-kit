// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: parameters

// Reduced from OpenCL 1.2 CTS test_convert_short3_char3:
//     ./conformance_test_conversions -3 short_char
__kernel void short3_char3_codegen(__global char *src, __global short *dest) {
  size_t gid = get_global_id(0);
  char3 in = vload3(gid, src);
  short3 out = CONVERT_FUNCTION(in);
  vstore3(out, gid, dest);
}
