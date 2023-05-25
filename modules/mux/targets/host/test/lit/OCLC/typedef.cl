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

// RUN: oclc -execute -enqueue vector_addition_scalar_real  -arg src1,1,2,3,4 -arg src2,10,20,30.4,90.6 -arg scalar,2 -print dst,4 -global 4 -local 4 %s > %t
// RUN: FileCheck < %t %s

typedef float REAL;
__kernel void vector_addition_scalar_real(__global REAL *src1, __global REAL *src2,
                                          __global REAL *dst, REAL scalar) {
    size_t gid = get_global_id(0);
    dst[gid] = src1[gid] + src2[gid] + scalar;
}

// CHECK: dst,13,24,35.4,96.6
