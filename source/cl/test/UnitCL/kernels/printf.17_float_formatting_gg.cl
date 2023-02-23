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

// SPIRV OPTIONS: -Xclang;-cl-ext=-cl_khr_fp64

kernel void float_formatting_gg(int size, global float* in) {
  // We are only printing from one workitem to make sure the order remains
  // constant
  if (0 == get_global_id(0)) {
    for (int i = 0; i < size; i++) {
      printf("*** SPACER ***\n");
      printf("%g\n", in[i]);
      printf("%G\n", in[i]);
      printf("%g, %g\n", in[i], in[i]);
      printf("%G, %G\n", in[i], in[i]);
      printf("%g hello world\n", in[i]);
      printf("%G hello world\n", in[i]);
      printf("%#g\n", in[i]);
      printf("%#G\n", in[i]);
      printf("%.0g\n", in[i]);
      printf("%.0G\n", in[i]);
      printf("%#.0g\n", in[i]);
      printf("%#.0G\n", in[i]);
      printf("%.3g\n", in[i]);
      printf("%.3G\n", in[i]);
      printf("%#.3g\n", in[i]);
      printf("%#.3G\n", in[i]);
      printf("%10.3g\n", in[i]);
      printf("%10.3G\n", in[i]);
      printf("%010.3g\n", in[i]);
      printf("%010.3G\n", in[i]);
      printf("%-10.3g\n", in[i]);
      printf("%-10.3G\n", in[i]);
      printf("%+10.3g\n", in[i]);
      printf("%+10.3G\n", in[i]);
      printf("%#10.3g\n", in[i]);
      printf("%#10.3G\n", in[i]);
    }
  }
}
