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
