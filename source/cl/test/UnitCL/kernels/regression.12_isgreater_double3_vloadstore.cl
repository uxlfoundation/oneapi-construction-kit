// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: double

#pragma OPENCL EXTENSION cl_khr_fp64 : enable
__kernel void isgreater_double3_vloadstore(__global double3 *inputA,
                                           __global double3 *inputB,
                                           __global long3 *output1,
                                           __global long3 *output2) {
  int tid = get_global_id(0);
  double3 tmpA = vload3(tid, (__global double *)inputA);
  double3 tmpB = vload3(tid, (__global double *)inputB);
  long3 result1 = isgreater(tmpA, tmpB);
  long3 result2 = tmpA > tmpB;
  vstore3(result1, tid, (__global long *)output1);
  vstore3(result2, tid, (__global long *)output2);
};
