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

// RUN: oclc -execute -enqueue vector_addition_half -arg src1,1,2,3,4 \
// RUN:   -arg src2,10,20,30,90 -print dst,4 -global 4 -local 4 %s | FileCheck %s

__kernel void vector_addition_half(__global half *src1, __global half *src2,
 __global half *dst) {
	size_t gid = get_global_id(0);
	float f1 = vload_half(gid, src1);
	float f2 = vload_half(gid, src2);
	float fd = f1 + f2;
	vstore_half(fd, gid, dst);
}

// CHECK: 11.000000,22.000000,33.000000,94.000000
