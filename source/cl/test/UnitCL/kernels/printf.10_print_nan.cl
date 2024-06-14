// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// If a device supports doubles, then printf will implicitly convert floats
// to doubles. If the device does not support doubles then this implicit
// conversion will not happen. However, as we generate and commit SPIR-V on
// a system that does support doubles, that implicit conversion is
// included in the SPIR-V bytecode and is used as part of those tests, even if
// the device doesn't support FP64 (such as Windows).
//
// To work around this issue, we disable the fp64 extension to ensure that the
// generated SPIR-V does not contain this implicit conversion.

// SPIRV OPTIONS: -Xclang;-cl-ext=-cl_khr_fp64;-Wno-format

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

  printf("\nas part of a longer format:\n");
  printf("lorem ipsum %f dolor sit amet", positive_nan);
}
