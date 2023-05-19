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

// REQUIRES: fp64
// RUN: %oclc -execute -enqueue vector_addition_scalar_double -compare dst,2,4,6.6,8 -arg src1,1,2,3.600000000000001,4 -arg "src2,range(0,3)" -arg scalar,1 %s -ulp-error 2 > %t
// RUN: %filecheck < %t %s

__kernel void vector_addition_scalar_double(double scalar, __global double *src1, __global double*src2,
 __global double *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = scalar + src1[gid] + src2[gid];
}

// CHECK: dst - match
