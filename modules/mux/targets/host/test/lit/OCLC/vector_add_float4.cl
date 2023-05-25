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

// RUN: oclc -execute -enqueue vector_addition_vec_float4 -arg src1,1,2,3,4.63,1,2,3,4.26,7,10,41,4,0,8,7,6 -arg src2,10,20,30.5,90.0,7,7,7,7.0,7,4,41,4,90,510,4,-6 -arg vec,2,8,98,3 -print dst,16 -global 4 -local 4 %s > %t
// RUN: FileCheck < %t %s

__kernel void vector_addition_vec_float4(float4 vec, __global float4 *src1, __global float4* src2,
 __global float4 *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = (float4) vec + src1[gid] + src2[gid];
}

// CHECK: dst,13,30,131.5,97.63,10,17,108,14.26,16,22,180,11,92,526,109,3
