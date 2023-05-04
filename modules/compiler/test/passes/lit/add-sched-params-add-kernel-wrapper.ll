; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes add-sched-params,add-kernel-wrapper,verify -S %s  | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: define internal void @add(ptr readonly %in, ptr byval(i32) %s) [[ATTRS:#[0-9]+]] {

; CHECK: define internal void @add.mux-sched-wrapper(ptr readonly %in, ptr byval(i32) %s, ptr [[WIATTRS:noalias nonnull align 8 dereferenceable\(40\)]] %wi-info, ptr [[WGATTRS:noalias nonnull align 8 dereferenceable\(104\)]] %wg-info) [[SCHED_ATTRS:#[0-9]+]] !mux_scheduled_fn [[SCHED_MD:\![0-9]+]] {

; CHECK: define void @add.mux-kernel-wrapper(ptr %packed-args, ptr [[WGATTRS]] %wg-info) [[WRAPPER_ATTRS:#[0-9]+]] !mux_scheduled_fn [[WRAPPER_MD:\![0-9]+]] {
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
