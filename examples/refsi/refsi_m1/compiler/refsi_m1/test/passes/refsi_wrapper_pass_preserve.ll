; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --device "%riscv_device" %s --passes refsi-wrapper,verify -S -opaque-pointers | %filecheck %s

target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
target triple = "riscv64-unknown-unknown-elf"

; Check the original attributes and metadata are preserved, but that the kernel entry point has been taken
; CHECK: define internal void @add(ptr nocapture readonly %packed_args, ptr nocapture readonly %wg_info) [[ATTRS:#[0-9]+]] !test [[TEST:\![0-9]+]] {

; Check that we've copied over all the parameter attributes, names, metadata and attributes
; CHECK: define void @add.refsi-wrapper(i64 %instance, i64 %slice, ptr nocapture readonly %packed_args, ptr nocapture readonly %wg_info) [[WRAPPER_ATTRS:#[0-9]+]] !test [[TEST]] {
; Check we call the original kernel with the right parameter attributes
; CHECK: call void @add(ptr nocapture readonly %packed_args, ptr nocapture readonly {{%.*}}) [[ATTRS]]
define void @add(ptr nocapture readonly %packed_args, ptr nocapture readonly %wg_info) #0 !test !0 {
  ret void
}

; Check we've preserved the original attributes, but that neither are marked 'alwaysinline'
; CHECK-DAG: attributes [[ATTRS]] = { noinline optsize "test"="attr" }
; CHECK-DAG: attributes [[WRAPPER_ATTRS]] = { noinline nounwind optsize "mux-base-fn-name"="add" "mux-kernel"="entry-point" "test"="attr" }

; CHECK-DAG: [[TEST]] = !{!"dummy"}

attributes #0 = { optsize noinline "mux-kernel"="entry-point" "test"="attr" }

!0 = !{!"dummy"}
