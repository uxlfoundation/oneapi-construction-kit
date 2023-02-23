; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes add-kernel-wrapper,verify -S %s -opaque-pointers | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: define internal void @add(ptr readonly %in, ptr byval(i32) %s, ptr noalias %wi-info, ptr noalias %wg-info) [[ATTRS:#[0-9]+]] !mux_scheduled_fn [[ADD_SCHED_PARAMS:\![0-9]+]] {

; Check we've preserved scheduling parameters, their names, and their attributes.
; Check we've dropped !mux_scheduled_fn metadata, which can't be ensured
; correct after this transformation.
; CHECK: define void @add.mux-kernel-wrapper(ptr %packed-args, ptr noalias %wg-info) [[WRAPPER_ATTRS:#[0-9]+]] !mux_scheduled_fn [[WRAPPER_SCHED_PARAMS:\![0-9]+]] {
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
