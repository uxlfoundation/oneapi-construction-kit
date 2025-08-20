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

// RUN: oclc -execute -enqueue vector_addition_vec_int2 -arg src1,1,2,3,4,5,6,7,8 -arg src2,10,20,30,90,40,50,60,70 -arg vec,2,8 -print dst,8 -global 4 -local 4 %s > %t
// RUN: FileCheck < %t %s

__kernel void vector_addition_vec_int2(int2 vec, __global int2 *src1, __global int2* src2,
 __global int2 *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = (int2) (vec.x + src1[gid].x + src2[gid].x, vec.y + src1[gid].y + src2[gid].y);
}

// CHECK: dst,13,30,35,102,47,64,69,86
