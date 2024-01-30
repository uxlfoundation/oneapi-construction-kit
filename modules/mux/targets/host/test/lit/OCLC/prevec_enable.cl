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

// REQUIRES: generic-target, ca_llvm_options
// RUN: env CA_LLVM_OPTIONS=-print-after=slp-vectorizer oclc -cl-options "-cl-vec=slp" -enqueue slp_test %s 2>&1 | FileCheck %s

__kernel void slp_test(__global int *out, __global int *in1, __global int *in2) {
  int x = get_global_id(0) * 8;

  int a0 = in1[x + 0] + in2[x + 0];
  int a1 = in1[x + 1] + in2[x + 1];
  int a2 = in1[x + 2] + in2[x + 2];
  int a3 = in1[x + 3] + in2[x + 3];
  int a4 = in1[x + 4] + in2[x + 4];
  int a5 = in1[x + 5] + in2[x + 5];
  int a6 = in1[x + 6] + in2[x + 6];
  int a7 = in1[x + 7] + in2[x + 7];

  out[x + 0] = a0;
  out[x + 1] = a1;
  out[x + 2] = a2;
  out[x + 3] = a3;
  out[x + 4] = a4;
  out[x + 5] = a5;
  out[x + 6] = a6;
  out[x + 7] = a7;
}

// This test makes sure that the prevectorization option creates a vector add.
// It's basically checking that -cl-vec=slp gets makes the pass pipeline run
// LLVM's SLP vectorizer, so this test is somewhat cumbersome.

// CHECK: add nsw <{{[0-9]+}} x i32>
