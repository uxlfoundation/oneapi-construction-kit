// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// This test is testing device-dependent OpenCL C behaviour, it doesnt make
// sense in SPIR or SPIR-V form.
// REQUIRES: nospir; nospirv;

__kernel void double_constant(__global float* out) {
  // 0x1.0p-126 is FLT_MIN (but without the f suffix)
  // 4294967296.0 is 2^32, chosen such that FLT_MIN/(2^32) is not representable
  // as a single precision float denormal. If doubles are supported, this
  // returns FLT_MIN. Otherwise, it will underflow to 0.
  out[0] = (0x1.0p-126 / 4294967296.0) * 4294967296.0;
}
