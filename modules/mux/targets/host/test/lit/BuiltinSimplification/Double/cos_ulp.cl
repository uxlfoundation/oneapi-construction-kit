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

// RUN: %oclc %s -execute -enqueue cos_fold -ulp-error 4 \
// RUN: -compare scalar_out,0.5403023058681398 \
// RUN: -compare vector_out,0.5403023058681398,-0.4161468365471424 \
// RUN: > %t
// RUN: %filecheck < %t %s

void kernel cos_fold(global double* scalar_out,
                     global double2* vector_out) {
  *scalar_out = cos(1.0);
  *vector_out = cos((double2)(1.0, 2.0));
}

// CHECK: scalar_out - match
// CHECK: vector_out - match
