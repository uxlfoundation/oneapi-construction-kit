// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// As with the other with_double_conversion printf test, this test is here to
// preserve test coverage of the implicit conversion that happens when you
// pass a float type smaller than double to printf on a platform that supports
// doubles. Since the test that was testing this with half has now had the
// double conversion removed we have this test with doubles explicitly enabled
// which will be skipped on platforms that don't support cl_khr_fp64.

// REQUIRES: double; half;
// SPIRV OPTIONS: -Xclang;-cl-ext=+cl_khr_fp64,+cl_khr_fp16
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

__kernel void half_with_double_conversion(__global half* input) {
  printf("%a\n", input[0]);
}
