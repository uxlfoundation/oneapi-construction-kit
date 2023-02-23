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

kernel void float_formatting_ff(int size, global float* in) {
  // We are only printing from one workitem to make sure the order remains
  // constant
  if (0 == get_global_id(0)) {
    for (int i = 0; i < size; i++) {
      printf("*** SPACER ***\n");
      printf("%f\n", in[i]);
      printf("%F\n", in[i]);
      printf("%f, %f\n", in[i], in[i]);
      printf("%F, %F\n", in[i], in[i]);
      printf("%f hello world\n", in[i]);
      printf("%F hello world\n", in[i]);
      printf("%f letter a\n", in[i]);
      printf("%F letter A\n", in[i]);
      printf("%f %%a percent-a\n", in[i]);
      printf("%F %%A percent-A\n", in[i]);
      printf("%.0f\n", in[i]);
      printf("%.1f\n", in[i]);
      printf("%.2f\n", in[i]);
      printf("%0.0f\n", in[i]);
      printf("%0.1f\n", in[i]);
      printf("%0.2f\n", in[i]);
      printf("%5.0f\n", in[i]);
      printf("%5.1f\n", in[i]);
      printf("%5.2f\n", in[i]);
      printf("%05.0f\n", in[i]);
      printf("%05.1f\n", in[i]);
      printf("%05.2f\n", in[i]);
      printf("%-5.0f\n", in[i]);
      printf("%-5.1f\n", in[i]);
      printf("%-5.2f\n", in[i]);
      printf("%+5.0f\n", in[i]);
      printf("%+5.1f\n", in[i]);
      printf("%+5.2f\n", in[i]);
    }
  }
}
