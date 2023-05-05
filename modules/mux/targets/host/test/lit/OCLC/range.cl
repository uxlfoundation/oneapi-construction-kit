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

// RUN: %oclc -execute -enqueue vector_addition_scalar_float    -arg src1,range(0,3) -arg src2,range(-0.666363,-0.666414,-0.000017) -arg scalar,2 -print dst,4 -print src1,4 -print src2,4 -global 4 -local 4 %s >& %t
// RUN: %filecheck < %t %s

__kernel void vector_addition_scalar_float(float scalar, __global float *src1, __global float*src2,
 __global float *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = scalar + src1[gid] + src2[gid];
}

// CHECK: dst,1.333637,2.33362,3.333603,4.333586
// CHECK-NEXT: src1,0,1,2,3
// CHECK-NEXT: src2,-0.666363,-0.66638,-0.666397,-0.666414
