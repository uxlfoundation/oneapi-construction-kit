// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// REQUIRES: noclc; nospirv;

// Note: This kernel isn't valid OpenCL C since __builtin_memcpy is a clang
//       specific intrinsic. Therefore we only have a SpirExecution test for
//       the generated SPIR, omitting the Execution test for this .cl file.
//       The SPIR-V version of this test uses spvasm files built manually using
//       the legacy SPIR generator, because modern clang can't compile this
//       test to SPIR (hence the nospirv above). See
//       spirv_regression.55_float_memcpy.spvasm{32|64}.
__kernel void float_memcpy(__global float *in, __global float *out, int size) {
  __builtin_memcpy((void*)out,(void*)in, size);
}
