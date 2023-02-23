// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: double

void kernel interleavedgroupcombine_safety(global double4* a, global double4* b,
                                           global double4* c, global double4* d,
                                           global double4* e,
                                           global double4* result) {
  size_t gid = get_global_id(0);
  double4 b4;
  global double4* tmp = b + gid;
  global double* tmp2 = (global double*)tmp;
  // Read half the vector
  b4[0] = (*tmp)[0];
  b4[1] = (*tmp)[1];

  // Synchronize
  barrier(CLK_GLOBAL_MEM_FENCE);
  *(tmp2 + 2) = 16.0;

  // Read the other half
  b4[2] = (*tmp)[2];
  b4[3] = (*tmp)[3];

  // Do stuff
  a[gid] -= b4 * c[gid] + d[gid] / e[gid];

  result[gid] = a[gid];
};
