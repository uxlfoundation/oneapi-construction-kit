; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %muxc --passes "add-kernel-wrapper<unpacked>,verify" -S %s | %filecheck %t

; Check that wrapped kernels are given 'alwaysinline' attributes, unless they
; have 'noinline' attributes.

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK-GE15: define internal spir_kernel void @add(ptr addrspace(1) %in, ptr addrspace(1) %out) #[[ATTR_INLINE:.+]] {
; CHECK-LT15: define internal spir_kernel void @add(i32 addrspace(1)* %in, i32 addrspace(1)* %out) #[[ATTR_INLINE:.+]] {

; CHECK-GE15: define internal spir_kernel void @[[K_NOINLINE:.*]](ptr addrspace(1) %in, ptr addrspace(1) %out) #[[ATTR_NOINLINE:.+]] {
; CHECK-LT15: define internal spir_kernel void @[[K_NOINLINE:.*]](i32 addrspace(1)* %in, i32 addrspace(1)* %out) #[[ATTR_NOINLINE:.+]] {

; CHECK-GE15: @add.mux-kernel-wrapper(ptr {{%.*}}) #[[WRAPPER_ATTRS_INLINE:[0-9]+]] !mux_scheduled_fn [[SCHED_MD:\![0-9]+]] {
; CHECK-GE15:  %1 = getelementptr %MuxPackedArgs.add, ptr %packed-args, i32 0, i32 0
; CHECK-GE15:  %in = load ptr addrspace(1), ptr %1, align 8
; CHECK-GE15:  %2 = getelementptr %MuxPackedArgs.add, ptr %packed-args, i32 0, i32 1
; CHECK-GE15:  %out = load ptr addrspace(1), ptr %2, align 8
; CHECK-GE15: call spir_kernel void @add(
; CHECK-LT15: @add.mux-kernel-wrapper(%MuxPackedArgs.add* {{%.*}}) #[[WRAPPER_ATTRS_INLINE:[0-9]+]] !mux_scheduled_fn [[SCHED_MD:\![0-9]+]] {
; CHECK-LT15:  %1 = getelementptr %MuxPackedArgs.add, %MuxPackedArgs.add* %packed-args, i32 0, i32 0
; CHECK-LT15:  %in = load i32 addrspace(1)*, i32 addrspace(1)** %1, align 8
; CHECK-LT15:  %2 = getelementptr %MuxPackedArgs.add, %MuxPackedArgs.add* %packed-args, i32 0, i32 1
; CHECK-LT15:  %out = load i32 addrspace(1)*, i32 addrspace(1)** %2, align 8
; CHECK-LT15: call spir_kernel void @add(
define spir_kernel void @add(i32 addrspace(1)* %in, i32 addrspace(1)* %out) #0 {
  %in.c = addrspacecast i32 addrspace(1)* %in to i32*
  %out.c = addrspacecast i32 addrspace(1)* %out to i32*
  ret void
}

; CHECK-GE15: @add_noinline.mux-kernel-wrapper(ptr {{%.*}}) #[[WRAPPER_ATTRS_NOINLINE:[0-9]+]] !mux_scheduled_fn [[SCHED_MD]] {
  ; CHECK-LT15: @add_noinline.mux-kernel-wrapper(%MuxPackedArgs.add_noinline* {{%.*}}) #[[WRAPPER_ATTRS_NOINLINE:[0-9]+]] !mux_scheduled_fn [[SCHED_MD]] {
; CHECK: call spir_kernel void @[[K_NOINLINE]](
define spir_kernel void @add_noinline(i32 addrspace(1)* %in, i32 addrspace(1)* %out) #1 {
  ret void
}

; CHECK-DAG: attributes #[[ATTR_INLINE]] = { alwaysinline }
; CHECK-DAG: attributes #[[ATTR_NOINLINE]] = { noinline }
; CHECK-DAG:  attributes #[[WRAPPER_ATTRS_INLINE]] =
; CHECK-SAME: "mux-base-fn-name"="add"
; CHECK-DAG:  attributes #[[WRAPPER_ATTRS_NOINLINE]] =
; CHECK-SAME: "mux-base-fn-name"="add_noinline"

; CHECK: [[SCHED_MD]] = !{i32 -1, i32 -1}

attributes #0 = { "mux-kernel"="entry-point" }
attributes #1 = { noinline "mux-kernel"="entry-point" }
