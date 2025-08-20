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

// RUN: oclc -execute -enqueue vector_addition_vec_short16 -arg src1,repeat(4,1,2,3,4,5,6,7,8,1,2,3,4,5,6,7,55) -arg src2,range(0,63) -arg vec,100,200,300,400,500,600,700,800,100,200,300,400,500,600,700,800 -print dst,64 -global 4 -local 4 %s > %t
// RUN: FileCheck < %t %s

__kernel void vector_addition_vec_short16(short16 vec, __global short16 *src1, __global short16* src2,
 __global short16 *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = (short16) vec + src1[gid] + src2[gid];
}

// CHECK: dst,101,203,305,407,509,611,713,815,109,211,313,415,517,619,721,870,117,219,321,423,525,627,729,831,125,227,329,431,533,635,737,886,133,235,337,439,541,643,745,847,141,243,345,447,549,651,753,902,149,251,353,455,557,659,761,863,157,259,361,463,565,667,769,918
