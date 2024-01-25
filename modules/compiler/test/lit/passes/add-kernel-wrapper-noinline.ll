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

; RUN: muxc --passes "add-kernel-wrapper<unpacked>,verify" -S %s | FileCheck %s

; Check that wrapped kernels are given 'alwaysinline' attributes, unless they
; have 'noinline' attributes.

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: define internal spir_kernel void @add(ptr addrspace(1) %in, ptr addrspace(1) %out) #[[ATTR_INLINE:.+]] {

; CHECK: define internal spir_kernel void @[[K_NOINLINE:.*]](ptr addrspace(1) %in, ptr addrspace(1) %out) #[[ATTR_NOINLINE:.+]] {

; CHECK: @add.mux-kernel-wrapper(ptr noundef nonnull dereferenceable(16) %packed-args) #[[WRAPPER_ATTRS_INLINE:[0-9]+]] !mux_scheduled_fn [[SCHED_MD:\![0-9]+]] {
; CHECK:  %1 = getelementptr %MuxPackedArgs.add, ptr %packed-args, i32 0, i32 0
; CHECK:  %in = load ptr addrspace(1), ptr %1, align 8
; CHECK:  %2 = getelementptr %MuxPackedArgs.add, ptr %packed-args, i32 0, i32 1
; CHECK:  %out = load ptr addrspace(1), ptr %2, align 8
; CHECK: call spir_kernel void @add(
define spir_kernel void @add(i32 addrspace(1)* %in, i32 addrspace(1)* %out) #0 {
  %in.c = addrspacecast i32 addrspace(1)* %in to i32*
  %out.c = addrspacecast i32 addrspace(1)* %out to i32*
  ret void
}

; CHECK: @add_noinline.mux-kernel-wrapper(ptr noundef nonnull dereferenceable(16) {{%.*}}) #[[WRAPPER_ATTRS_NOINLINE:[0-9]+]] !mux_scheduled_fn [[SCHED_MD]] {
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
