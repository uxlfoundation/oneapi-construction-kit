// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// If a device supports doubles, then printf will implicitly convert floats
// to doubles. If the device does not support doubles then this implicit
// conversion will not happen. However, as we generate and commit SPIR-V on
// a system that does support doubles, that implicit conversion is
// included in the SPIR-V bytecode and is used as part of those tests, even if
// the device doesn't support FP64 (such as Windows).
//
// To work around this issue, we disable the fp64 extension to ensure that the
// generated SPIR-V does not contain this implicit conversion.

// DEFINITIONS: -DNUM_INPUTS=32
// SPIRV OPTIONS: -Xclang;-cl-ext=-cl_khr_fp64

#ifndef NUM_INPUTS
#define NUM_INPUTS 1
#endif

kernel void floats(global float* in, global float* out) {
  size_t gid = get_global_id(0);
  out[gid] = in[gid] * in[gid];
  int i;
  // We are only printing from one workitem to make sure the order remains
  // constant
  if (gid == 0) {
    for (i = 0; i < NUM_INPUTS; ++i) {
      printf("%#16.1A\n", in[i] * in[i]);
    }
  }
}
