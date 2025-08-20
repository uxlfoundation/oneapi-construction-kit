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

// RUN: oclc -cl-options "-cl-opt-disable" -execute -enqueue vector_multiply_local -arg src,1,2,3,4,5,6,7,8,9,10 -arg tmp,40 -print dst,10 -global 10 -local 2 %s > %t
// RUN: FileCheck < %t %s

__kernel void vector_multiply_local(__global int *src, __local int *tmp, __global int *dst) {
    size_t gid = get_global_id(0);
    size_t lid = get_local_id(0);
    tmp[lid] = src[gid] * 2;
    dst[gid] = tmp[lid] * 2;
}

// CHECK: dst,4,8,12,16,20,24,28,32,36,40
