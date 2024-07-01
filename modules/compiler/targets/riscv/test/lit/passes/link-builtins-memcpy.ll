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

; RUN: muxc --device "%riscv_device" --passes link-builtins,verify -S %s | FileCheck %s

target triple = "riscv64-unknown-unknown-elf"
target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128"

; CHECK: define linkonce dso_local spir_func noundef ptr @memcpy(

define void @memcpy_kernel_32_p0(ptr %out, ptr %in, i32 %size) {
entry:
  call void @llvm.memcpy.p0.p0.i32(ptr %out, ptr %in, i32 %size, i1 false)
  ret void
}

define void @memcpy_kernel_32_p1(ptr addrspace(1) %out, ptr addrspace(1) %in, i32 %size) {
entry:
  call void @llvm.memcpy.p1.p1.i32(ptr addrspace(1) %out, ptr addrspace(1) %in, i32 %size, i1 false)
  ret void
}

define void @memcpy_kernel_64_p0_p2(ptr %out, ptr addrspace(2) %in, i64 %size) {
entry:
  call void @llvm.memcpy.p0.p2.i64(ptr %out, ptr addrspace(2) %in, i64 %size, i1 false)
  ret void
}

define void @memcpy_kernel_32_p2_p1(ptr addrspace(2) %out, ptr addrspace(1) %in, i32 %size) {
entry:
  call void @llvm.memcpy.p2.p1.i32(ptr addrspace(2) %out, ptr addrspace(1) %in, i32 %size, i1 false)
  ret void
}

declare void @llvm.memcpy.p0.p0.i32(ptr  noalias nocapture writeonly, ptr  noalias nocapture readonly, i32, i1 immarg)
declare void @llvm.memcpy.p1.p1.i32(ptr addrspace(1) noalias nocapture writeonly, ptr addrspace(1) noalias nocapture readonly, i32, i1 immarg)
declare void @llvm.memcpy.p0.p2.i64(ptr  noalias nocapture writeonly, ptr addrspace(2) noalias nocapture readonly, i64, i1 immarg)
declare void @llvm.memcpy.p2.p1.i32(ptr addrspace(2) noalias nocapture writeonly, ptr addrspace(1) noalias nocapture readonly, i32, i1 immarg)
