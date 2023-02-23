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

__kernel void print_inf(void) {
  // single specifiers
  float negative_inf = copysign(INFINITY, -1.0f);
  float positive_inf = copysign(INFINITY, 1.0f);
  printf("f and F specifiers:\n%f", positive_inf);
  printf("%f", negative_inf);
  printf("%F", positive_inf);
  printf("%F", negative_inf);

  printf("\ne and E specifiers:\n%e", positive_inf);
  printf("%e", negative_inf);
  printf("%E", positive_inf);
  printf("%E", negative_inf);

  printf("\ng and G specifiers:\n%g", positive_inf);
  printf("%g", negative_inf);
  printf("%G", positive_inf);
  printf("%G", negative_inf);

  printf("\na and A specifiers:\n%a", positive_inf);
  printf("%a", negative_inf);
  printf("%A", positive_inf);
  printf("%A", negative_inf);

  printf("\ncomplex specifiers:\n");
  printf("%.2f", positive_inf);
  printf("%08.2f", negative_inf);
  printf("%-8.2f", positive_inf);
  printf("%-#20.15e", negative_inf);
  printf("%.6a", positive_inf);
  printf("% G", positive_inf);
  printf("% G", negative_inf);
  printf("%+f", positive_inf);
  printf("%+f", negative_inf);
  printf("% +A", positive_inf);
}
