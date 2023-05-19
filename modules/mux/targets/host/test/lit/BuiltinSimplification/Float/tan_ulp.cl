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

// RUN: %oclc %s -execute -enqueue tan_fold -ulp-error 5 \
// RUN: -compare scalar_out,1.5574077246549023 \
// RUN: -compare vector_out,1.5574077246549023,2.2913879924374863 \
// RUN: > %t
// RUN: %filecheck < %t %s

void kernel tan_fold(global float* scalar_out,
                     global float2* vector_out) {
  *scalar_out = tan(1.0f);
  *vector_out = tan((float2)(1.0f, 42.0f));
}

// CHECK: scalar_out - match
// CHECK: vector_out - match
