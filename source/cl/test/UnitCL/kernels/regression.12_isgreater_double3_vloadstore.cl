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

// REQUIRES: double

#pragma OPENCL EXTENSION cl_khr_fp64 : enable
__kernel void isgreater_double3_vloadstore(__global double3 *inputA,
                                           __global double3 *inputB,
                                           __global long3 *output1,
                                           __global long3 *output2) {
  int tid = get_global_id(0);
  double3 tmpA = vload3(tid, (__global double *)inputA);
  double3 tmpB = vload3(tid, (__global double *)inputB);
  long3 result1 = isgreater(tmpA, tmpB);
  long3 result2 = tmpA > tmpB;
  vstore3(result1, tid, (__global long *)output1);
  vstore3(result2, tid, (__global long *)output2);
};
