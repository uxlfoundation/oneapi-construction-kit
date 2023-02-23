// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: double

#pragma OPENCL EXTENSION cl_khr_fp64 : enable
__kernel void fract_double3(__global double* in, __global double* out,
                            __global double* out2) {
  size_t i = get_global_id(0);
  // We use get_global_size() to make sure the condition is always true,
  // without the compiler knowing it.
  if (i < get_global_size(0)) {
    double3 f0 = vload3(0, in + 3 * i);
#ifdef DEBUG_PRINTFS
    printf("[%d] f0_a: <%f, %f, %f>\n", i, f0.s0, f0.s1, f0.s2);
#endif
    double3 iout = NAN;
    f0 = fract(f0, &iout);
#ifdef DEBUG_PRINTFS
    printf("[%d] f0_b: <%f, %f, %f>\n", i, f0.s0, f0.s1, f0.s2);
#endif
    vstore3(f0, 0, out + 3 * i);
#ifdef DEBUG_PRINTFS
    printf("[%d] iout: <%f, %f, %f>\n", i, iout.s0, iout.s1, iout.s2);
#endif
    vstore3(iout, 0, out2 + 3 * i);
  }
}
