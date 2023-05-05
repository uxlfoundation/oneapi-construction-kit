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

// RUN: %oclc -execute -enqueue vector_addition_scalar_long    -arg src1,range(9223372036854775807,9223372036854775804,-1) -arg src2,-1,-2,-3,-4 -arg scalar,1 -print dst,4 -print src1,4 -print src2,4 -global 4 -local 4 %s > %t
// RUN: %filecheck < %t %s

__kernel void vector_addition_scalar_long(long scalar, __global long *src1, __global long*src2,
 __global long *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = scalar + src1[gid] + src2[gid];
}

// CHECK: dst,9223372036854775807,9223372036854775805,9223372036854775803,9223372036854775801
// CHECK-NEXT: src1,9223372036854775807,9223372036854775806,9223372036854775805,9223372036854775804
// CHECK-NEXT: src2,-1,-2,-3,-4
