; Copyright (C) Codeplay Software Limited
;
; Licensed under the Apache License, Version 2.0 (the "License") with LLVM
; Exceptions; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

; RUN: muxc --passes add-kernel-wrapper,verify -S %s  | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: define internal spir_kernel void @add(ptr addrspace(1) readonly %in, ptr addrspace(1) writeonly %out, i8 signext %x, ptr byval(i32) %s) [[ATTRS:#[0-9]+]] !test [[TEST:\![0-9]+]] {

; Check we've copied across all the metadata, and stolen the entry-point metadata
; CHECK: define spir_kernel void @orig.mux-kernel-wrapper(ptr noundef nonnull dereferenceable(21) %packed-args) [[WRAPPER_ATTRS:#[0-9]+]] !test [[TEST]] !mux_scheduled_fn [[SCHED_MD:\![0-9]+]] {
; Check we're calling the original kernel with the right attributes
; CHECK: call spir_kernel void @add(ptr addrspace(1) readonly %in, ptr addrspace(1) writeonly %out, i8 signext %x, ptr byval(i32) %s) [[ATTRS]]
define spir_kernel void @add(ptr addrspace(1) readonly %in, ptr addrspace(1) writeonly %out, i8 signext %x, ptr byval(i32) %s) #0 !test !0 {
  ret void
}

; Check we've preserved the attributes but added 'alwaysinline'
; CHECK-DAG: attributes [[ATTRS]] = { alwaysinline optsize "foo"="bar" "mux-base-fn-name"="orig" }
; Check we've preserved the original attributes but added 'nounwind'
; CHECK-DAG: attributes [[WRAPPER_ATTRS]] = { nounwind optsize "foo"="bar" "mux-base-fn-name"="orig" "mux-kernel" }

; CHECK-DAG: [[TEST]] = !{!"test"}
; CHECK-DAG: [[SCHED_MD]] = !{i32 -1, i32 -1}

attributes #0 = { optsize "foo"="bar" "mux-base-fn-name"="orig" "mux-kernel" }

!0 = !{!"test"}
