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

// It shouldn't matter which device we run this test on.
// RUN: muxc --device-idx 0 -x cl -cl-options '-cl-kernel-arg-info' < %s | FileCheck %s

void kernel foo(const int a) {}

// CHECK: !{{{(ptr|void \(i32\)\*)}} @foo, [[ADDR_SPACE:![0-9]+]], [[ACCESS_QUAL:![0-9]+]], [[TYPE:![0-9]+]], [[BASE_TYPE:![0-9]+]], [[TYPE_QUAL:![0-9]+]], [[ARG_NAME:![0-9]+]]}
// CHECK: [[ADDR_SPACE]] = !{!"kernel_arg_addr_space", i32 0}
// CHECK: [[ACCESS_QUAL]] = !{!"kernel_arg_access_qual", !"none"}
// CHECK: [[TYPE]] = !{!"kernel_arg_type", !"int"}
// CHECK: [[BASE_TYPE]] = !{!"kernel_arg_base_type", !"int"}
// CHECK: [[TYPE_QUAL]] = !{!"kernel_arg_type_qual", !""}
// CHECK: [[ARG_NAME]] = !{!"kernel_arg_name", !"a"}
