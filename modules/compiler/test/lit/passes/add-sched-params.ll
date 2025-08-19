; Copyright (C) Codeplay Software Limited
;
; Licensed under the Apache License, Version 2.0 (the "License") with LLVM
; Exceptions; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

; RUN: muxc --passes add-sched-params,verify -S %s  | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

declare i64 @__mux_get_global_id(i32)

; Check that @foo is given scheduling parameters because it is a kernel and so
; must present a consistent ABI
; CHECK: define void @kernel.mux-sched-wrapper(ptr [[WIATTRS:noalias nonnull align 8 dereferenceable\(40\)]] %wi-info, ptr [[WGATTRS:noalias nonnull align 8 dereferenceable\(104\)]] %wg-info) 
; CHECK-SAME: [[KERNEL_ATTRS:#[0-9]+]] !mux_scheduled_fn [[KERNEL_SCHED_MD:\![0-9]+]] {
define void @kernel() #0 {
  ret void
}

; Check that @foo is given scheduling parameters because it uses a work-item builtin
; CHECK: define void @foo.mux-sched-wrapper(ptr addrspace(1) %p, ptr [[WIATTRS]] %wi-info, ptr [[WGATTRS]] %wg-info) 
; CHECK-SAME: [[FOO_ATTRS:#[0-9]+]] !mux_scheduled_fn [[FOO_SCHED_MD:\![0-9]+]] {
define void @foo(ptr addrspace(1) %p) {
  %call = tail call i64 @__mux_get_global_id(i32 0)
  ret void
}

; Check that @bar isn't given scheduling parameters because it doesn't use a work-item builtin
; CHECK-NOT: define void @bar.mux-sched-wrapper
define void @bar(ptr addrspace(1) %p) {
  ret void
}

; CHECK-DAG: attributes [[FOO_ATTRS]] = { "mux-base-fn-name"="foo" }
; CHECK-DAG: attributes [[KERNEL_ATTRS]] = { "mux-base-fn-name"="kernel" "mux-kernel"="entry-point" }

attributes #0 = { "mux-kernel"="entry-point" }

; CHECK-DAG: [[FOO_SCHED_MD]] = !{i32 1, i32 2}
; CHECK-DAG: [[KERNEL_SCHED_MD]] = !{i32 0, i32 1}
