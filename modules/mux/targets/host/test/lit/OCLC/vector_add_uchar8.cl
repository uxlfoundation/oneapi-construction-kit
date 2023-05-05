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

// RUN: %oclc -execute -enqueue vector_addition_vec_uchar8 -arg src1,1,2,3,4,5,6,7,8,1,2,3,4,5,6,7,8,1,2,3,4,5,6,7,8,1,2,3,4,5,6,7,8 -arg src2,repeat(32,randint(0,15)) -arg vec,100,200,100,200,100,100,100,100 -print dst,32 -global 4 -local 4 %s > %t
// RUN: %filecheck < %t %s

__kernel void vector_addition_vec_uchar8(uchar8 vec, __global uchar8 *src1, __global uchar8* src2,
 __global uchar8 *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = (uchar8) vec + src1[gid] + src2[gid];
}

// CHECK: dst,113,206,114,219,105,112,111,108,109,207,107,212,107,114,115,121,108,208,114,207,108,111,121,110,104,205,114,219,117,107,114,112
