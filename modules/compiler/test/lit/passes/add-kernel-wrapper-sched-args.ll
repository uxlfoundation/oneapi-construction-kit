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

; CHECK: define internal void @add(ptr readonly %in, ptr byval(i32) %s, ptr noalias %wi-info, ptr noalias %wg-info) [[ATTRS:#[0-9]+]] !mux_scheduled_fn [[ADD_SCHED_PARAMS:\![0-9]+]] {

; Check we've preserved scheduling parameters, their names, and their attributes.
; Check we've dropped !mux_scheduled_fn metadata, which can't be ensured
; correct after this transformation.
; CHECK: define void @add.mux-kernel-wrapper(ptr noundef nonnull dereferenceable(12) %packed-args, ptr noalias %wg-info) [[WRAPPER_ATTRS:#[0-9]+]] !mux_scheduled_fn [[WRAPPER_SCHED_PARAMS:\![0-9]+]] {
; Check we're calling the original kernel, passing through the scheduling
; parameters and with the right attributes
; CHECK: call void @add(ptr readonly %in, ptr byval(i32) %s, ptr noalias %wi-info, ptr noalias %wg-info) [[ATTRS]]
define void @add(ptr readonly %in, ptr byval(i32) %s, ptr noalias %wi-info, ptr noalias %wg-info) #0 !mux_scheduled_fn !0 {
  ret void
}

; Check we've preserved the attributes, added 'alwaysinline', and dropped
; "mux-kernel" - the wrapper is our kernel now.
; CHECK-DAG: attributes [[ATTRS]] = { alwaysinline optsize }
; Check we've preserved the original attributes but added 'nounwind'
; CHECK-DAG: attributes [[WRAPPER_ATTRS]] = { nounwind optsize "mux-base-fn-name"="add" "mux-kernel" }

; CHECK-DAG: [[ADD_SCHED_PARAMS]] = !{i32 2, i32 3}
; CHECK-DAG: [[WRAPPER_SCHED_PARAMS]] = !{i32 -1, i32 1}

attributes #0 = { "mux-kernel" optsize }

!0 = !{i32 2, i32 3}
