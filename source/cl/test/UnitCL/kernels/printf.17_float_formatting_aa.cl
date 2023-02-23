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

kernel void float_formatting_aa(int size, global float* in) {
  // We are only printing from one workitem to make sure the order remains
  // constant
  if (0 == get_global_id(0)) {
    for (int i = 0; i < size; i++) {
      printf("*** SPACER ***\n");
      printf("%a\n", in[i]);
      printf("%A\n", in[i]);
      printf("%a, %a\n", in[i], in[i]);
      printf("%A, %A\n", in[i], in[i]);
      printf("%a hello world\n", in[i]);
      printf("%A hello world\n", in[i]);
      printf("%#a\n", in[i]);
      printf("%#A\n", in[i]);
      printf("%10a\n", in[i]);
      printf("%10A\n", in[i]);
      printf("%#10a\n", in[i]);
      printf("%#10A\n", in[i]);
      printf("%.0a\n", in[i]);
      printf("%.0A\n", in[i]);
      printf("%#.0a\n", in[i]);
      printf("%#.0A\n", in[i]);
      printf("%.3a\n", in[i]);
      printf("%.3A\n", in[i]);
      printf("%#.3a\n", in[i]);
      printf("%#.3A\n", in[i]);
      printf("%010.3a\n", in[i]);
      printf("%010.3A\n", in[i]);
      printf("%-10a hello world\n", in[i]);
      printf("%-10A hello world\n", in[i]);
      printf("%+10.3a\n", in[i]);
      printf("%+10.3A\n", in[i]);
      printf("%#10.3a\n", in[i]);
      printf("%#10.3A\n", in[i]);
      printf("%12.3a\n", in[i]);
      printf("%12.3A\n", in[i]);
      printf("%#12.3a\n", in[i]);
      printf("%#12.3A\n", in[i]);
    }
  }
}
