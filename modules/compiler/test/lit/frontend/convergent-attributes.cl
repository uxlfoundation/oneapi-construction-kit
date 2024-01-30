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

// During investigation of CA-3691 it was discovered that clang sets the
// `convergent` attribute on all functions when the source language is OpenCL.
// This is overly pessimistic. However, certain workgroup functions e.g.
// barrier() *do* need that attribute. This tests that we see convergent in some
// of the right places (it's not an exhaustive test)

// LLVM attributes are uniqued and shared, yet FileCheck doesn't support
// negative matches for DAG checks. Thus we need this rigmarole of multiple
// independent FileCheck runs
//
// It shouldn't matter which device we run this test on.
// RUN: muxc --device-idx 0 %s -o %t.ll

// RUN: FileCheck %s --check-prefixes=CHECK-FUNCTION,CHECK < %t.ll
// RUN: FileCheck %s --check-prefixes=CHECK-GGID,CHECK < %t.ll
// RUN: FileCheck %s --check-prefixes=CHECK-GGID-CALL,CHECK < %t.ll
// RUN: FileCheck %s --check-prefixes=CHECK-GLID,CHECK < %t.ll
// RUN: FileCheck %s --check-prefixes=CHECK-GLID-CALL,CHECK < %t.ll
// RUN: FileCheck %s --check-prefixes=CHECK-KERNEL,CHECK < %t.ll
// RUN: FileCheck %s --check-prefixes=CHECK-BARRIER,CHECK < %t.ll
// RUN: FileCheck %s --check-prefixes=CHECK-BARRIER-CALL,CHECK < %t.ll

// CHECK: define dso_local spir_func i32 @function() #[[FUNCTION_ATTRS:[0-9]+]]
int __attribute__((noinline)) function(void) {
  return get_local_id(0) + 1;
}

// CHECK: declare spir_func {{.*}} @_Z12get_local_idj(i32 noundef) #[[GLID_FN_ATTRS:[0-9]+]]

// CHECK: define dso_local spir_kernel void @do_stuff{{.*}} #[[KERNEL_ATTRS:[0-9]+]]
// CHECK: call spir_func {{.*}}@_Z13get_global_idj(i32 noundef 0) #[[GGID_CALL_ATTRS:[0-9]+]]
// CHECK: call spir_func {{.*}}_Z7barrierj(i32 noundef 1) #[[BARRIER_CALL_ATTRS:[0-9]+]]
// CHECK: call spir_func {{.*}}function() #[[FUNCTION_CALL_ATTRS:[0-9]+]]
__kernel void do_stuff(int __global *out, const int __global *in) {
  size_t i = get_global_id(0);
  barrier(CLK_LOCAL_MEM_FENCE);
  out[i] += in[function()];
}

// CHECK: declare spir_func {{.*}}@_Z13get_global_idj(i32 noundef){{.*}} #[[GGID_FN_ATTRS:[0-9]+]]
// CHECK: declare spir_func {{.*}}@_Z7barrierj(i32 noundef){{.*}} #[[BARRIER_ATTRS:[0-9]+]]

// CHECK-FUNCTION-NOT: attributes #[[FUNCTION_ATTRS]] = {{.*}}convergent
// CHECK-GGID-NOT: attributes #[[GGID_CALL_ATTRS]] = {{.*}}convergent
// CHECK-GGID-CALL-NOT: attributes #[[GGID_CALL_ATTRS]] = {{.*}}convergent
// CHECK-GLID-NOT: attributes #[[GGID_CALL_ATTRS]] = {{.*}}convergent
// CHECK-GLID-CALL-NOT: attributes #[[GGID_CALL_ATTRS]] = {{.*}}convergent
// CHECK-KERNEL-NOT: attributes #[[KERNEL_ATTRS]] = {{.*}}convergent
// CHECK-BARRIER: attributes #[[BARRIER_ATTRS]] = {{{.*}}convergent
// CHECK-BARRIER-CALL: attributes #[[BARRIER_CALL_ATTRS]] = {{{.*}}convergent
