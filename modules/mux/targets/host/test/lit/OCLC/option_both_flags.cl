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

// REQUIRES: debug
// RUN: oclc -cl-options "--dummy-host-flag --dummy-host-flag2" -enqueue add %s -stage cl_snapshot_host_scheduled > %t
// RUN: FileCheck < %t %s

__kernel void add(__global int* input1,
                  __global int* input2,
                  __global int* output) {
  int gid = get_local_id(0);
  output[gid] = input1[gid] + input2[gid];
}

// CHECK: !host.build_options = !{[[MD_STRING:![0-9]+]]}
// CHECK: [[MD_STRING]] = !{!"--dummy-host-flag,;--dummy-host-flag2,"}
