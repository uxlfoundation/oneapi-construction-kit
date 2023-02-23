// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// Test derived from OpenCL 1.2 CTS vecalign/vec_align_struct local test:
//   ./conformance_test_vecalign vec_align_struct
__kernel void local_vecalign(__global ulong *dst) {
  __local char2 vecx;
  int gid = get_global_id(0);
  dst[gid] = (ulong)((__local char *)&(vecx)) % 2u;
}
