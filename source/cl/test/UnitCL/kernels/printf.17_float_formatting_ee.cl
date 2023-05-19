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

// SPIRV OPTIONS: -Xclang;-cl-ext=-cl_khr_fp64

kernel void float_formatting_ee(int size, global float* in) {
  // We are only printing from one workitem to make sure the order remains
  // constant
  if (0 == get_global_id(0)) {
    for (int i = 0; i < size; i++) {
      printf("*** SPACER ***\n");
      printf("%e\n", in[i]);
      printf("%E\n", in[i]);
      printf("%e, %e\n", in[i], in[i]);
      printf("%E, %E\n", in[i], in[i]);
      printf("%e hello world\n", in[i]);
      printf("%E hello world\n", in[i]);
      printf("%.0e\n", in[i]);
      printf("%.0E\n", in[i]);
      printf("%.3e\n", in[i]);
      printf("%.3E\n", in[i]);
      printf("%11.3e\n", in[i]);
      printf("%11.3E\n", in[i]);
      printf("%011.3e\n", in[i]);
      printf("%011.3E\n", in[i]);
      printf("%-11.3e hello world\n", in[i]);
      printf("%-11.3E hello world\n", in[i]);
      printf("%+11.3e\n", in[i]);
      printf("%+11.3E\n", in[i]);
    }
  }
}
