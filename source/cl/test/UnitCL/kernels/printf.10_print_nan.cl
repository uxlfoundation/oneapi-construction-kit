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

__kernel void print_nan(void) {
  // single specifiers
  float negative_nan = copysign(NAN, -1.0f);
  float positive_nan = copysign(NAN, 1.0f);
  printf("f and F specifiers:\n%f", positive_nan);
  printf("%f", negative_nan);
  printf("%F", positive_nan);
  printf("%F", negative_nan);

  printf("\ne and E specifiers:\n%e", positive_nan);
  printf("%e", negative_nan);
  printf("%E", positive_nan);
  printf("%E", negative_nan);

  printf("\ng and G specifiers:\n%g", positive_nan);
  printf("%g", negative_nan);
  printf("%G", positive_nan);
  printf("%G", negative_nan);

  printf("\na and A specifiers:\n%a", positive_nan);
  printf("%a", negative_nan);
  printf("%A", positive_nan);
  printf("%A", negative_nan);

  printf("\ncomplex specifiers:\n");
  printf("%.2f", positive_nan);
  printf("%08.2f", negative_nan);
  printf("%-8.2f", positive_nan);
  printf("%-#20.15e", negative_nan);
  printf("%.6a", positive_nan);
  printf("% G", positive_nan);
  printf("% G", negative_nan);
  printf("%+f", positive_nan);
  printf("%+f", negative_nan);
  printf("% +A", positive_nan);
}
