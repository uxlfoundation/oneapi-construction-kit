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

// RUN: clc -d %host_ca_host_cl_device_name -cl-kernel-arg-info -cl-wfv=always -cl-llvm-stats -n -- %s 2> %t
// RUN: FileCheck < %t %s

__attribute__((reqd_work_group_size(16, 1, 1)))
__kernel void add(__global int* input1,
                  __global int* input2,
                  __global int* output) {
  int gid = get_local_id(0);
  output[gid] = input1[gid] + input2[gid];
}

// not going to check the number of vector instructions, since target-dependent subwidening
// doesn't necessarily result in 1 vector instruction per packetized instruction.
// CHECK: {{3|6|12}} vecz{{ *}}- Number of vector loads and stores in the vectorized kernel [ID#V0B]
// CHECK: 16 vecz{{ *}}- Vector width of the vectorized kernel [ID#V12]
// CHECK: 4 vecz-packetization{{ *}}- Number of instructions packetized [ID#P00]
