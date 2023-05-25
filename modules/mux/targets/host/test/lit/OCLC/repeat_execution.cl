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

// FIXME: DISABLED (See CA-1147)
// UNSUPPORTED: true

// RUN: oclc -execute -enqueue vector_addition_scalar_int    -arg src1,repeat(3,{repeat(4,randint(-100,100))}) -arg src2,10,20,30,40 -arg scalar,{1},{2},{3} -global {32,1,1},{64,4,1},{128,4,1} -local {16,1,1},{32,2,1},{64,4,1} -print dst,4 -repeat-execution 3 %s > %t
// RUN: FileCheck < %t %s

__kernel void vector_addition_scalar_int(int scalar, __global int *src1, __global int*src2,
 __global int *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = scalar + src1[gid] + src2[gid];
}

// CHECK: dst,69,-28,73,131
// CHECK-NEXT: dst,-84,4,-17,-53
// CHECK-NEXT: dst,17,-7,-11,55
