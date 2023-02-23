; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes add-kernel-wrapper,verify -S %s -opaque-pointers | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: define internal spir_kernel void @add(ptr addrspace(1) readonly %in, ptr addrspace(1) writeonly %out, i8 signext %x, ptr byval(i32) %s) [[ATTRS:#[0-9]+]] !test [[TEST:\![0-9]+]] {

; Check we've copied across all the metadata, and stolen the entry-point metadata
; CHECK: define spir_kernel void @orig.mux-kernel-wrapper(ptr %packed-args) [[WRAPPER_ATTRS:#[0-9]+]] !test [[TEST]] !mux_scheduled_fn [[SCHED_MD:\![0-9]+]] {
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
