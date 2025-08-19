// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// As with the other with_double_conversion printf test, this test is here to
// preserve test coverage of the implicit conversion that happens when you
// pass a float type smaller than double to printf on a platform that supports
// doubles. Since the test that was testing this with half has now had the
// double conversion removed we have this test with doubles explicitly enabled
// which will be skipped on platforms that don't support cl_khr_fp64.

// REQUIRES: double; half;
// SPIRV OPTIONS: -Xclang;-cl-ext=+cl_khr_fp64,+cl_khr_fp16
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

__kernel void half_with_double_conversion(__global half* input) {
  printf("%a\n", input[0]);
}
