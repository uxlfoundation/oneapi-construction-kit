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

// RUN: %oclc -execute -enqueue vector_addition_scalar_float   -arg src1,rand(0.0,5.5),rand(-4.3,4.3),rand(-2.1,-0.01),4 -arg src2,repeat(4,rand(0,20.3)) -arg scalar,2 -print dst,4 -print src1,4 -print src2,4 -global 4 -local 4 -seed 2002 %s > %t
// RUN: %filecheck < %t %s

__kernel void vector_addition_scalar_float(float scalar, __global float *src1, __global float*src2,
 __global float *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = scalar + src1[gid] + src2[gid];
}

// CHECK: dst,16.40238,13.36131,19.91158,16.69472
// CHECK-NEXT: src1,0.4408496,3.247275,-0.7590057,4
// CHECK-NEXT: src2,13.96153,8.114035,18.67058,10.69472
