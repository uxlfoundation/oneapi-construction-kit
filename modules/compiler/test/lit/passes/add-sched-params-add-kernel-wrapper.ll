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

; RUN: muxc --passes add-sched-params,add-kernel-wrapper,verify -S %s  | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: define internal void @add(ptr readonly %in, ptr byval(i32) %s) [[ATTRS:#[0-9]+]] {

; CHECK: define internal void @add.mux-sched-wrapper(ptr readonly %in, ptr byval(i32) %s, ptr [[WIATTRS:noalias nonnull align 8 dereferenceable\(40\)]] %wi-info, ptr [[WGATTRS:noalias nonnull align 8 dereferenceable\(104\)]] %wg-info) [[SCHED_ATTRS:#[0-9]+]] !mux_scheduled_fn [[SCHED_MD:\![0-9]+]] {

; CHECK: define void @add.mux-kernel-wrapper(ptr noundef nonnull dereferenceable(12) %packed-args, ptr [[WGATTRS]] %wg-info) [[WRAPPER_ATTRS:#[0-9]+]] !mux_scheduled_fn [[WRAPPER_MD:\![0-9]+]] {
; Check we're initializing the work-item info on the stack
; CHECK: %wi-info = alloca %MuxWorkItemInfo, align 8
; Check we're calling the original kernel, passing through the scheduling
; parameters and with the right attributes
; CHECK: call void @add.mux-sched-wrapper(ptr readonly %in, ptr byval(i32) %s, ptr [[WIATTRS]] %wi-info, ptr [[WGATTRS]] %wg-info) [[SCHED_ATTRS]]
define void @add(ptr readonly %in, ptr byval(i32) %s) #0 {
  ret void
}

; CHECK-DAG: attributes [[ATTRS]] = { optsize }
; CHECK-DAG: attributes [[SCHED_ATTRS]] = { alwaysinline optsize "mux-base-fn-name"="add" }
; CHECK-DAG: attributes [[WRAPPER_ATTRS]] = { nounwind optsize "mux-base-fn-name"="add" "mux-kernel"="entry-point" }

; CHECK: !mux-scheduling-params = !{[[PARAMS_MD:\![0-9]+]]}

; CHECK-DAG: [[PARAMS_MD]] = !{!"MuxWorkItemInfo", !"MuxWorkGroupInfo"}
; CHECK-DAG: [[SCHED_MD]] = !{i32 2, i32 3}
; CHECK-DAG: [[WRAPPER_MD]] = !{i32 -1, i32 1}

attributes #0 = { "mux-kernel"="entry-point" optsize }
