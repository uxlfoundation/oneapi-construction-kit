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

; RUN: muxc --passes replace-mem-intrins,verify -S %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; Note we use old style non opaque pointers as input as the utility replace memmove 
; code throws asserts with the opaque pointers on llvm 14

; CHECK-NOT: call void @llvm.memmove.p0{{.*}}.p0{{.*}}.i32
; CHECK-NOT: call void @llvm.memmove.p1{{.*}}.p1{{.*}}.i32
; The next two are not currently handled so check they still exist - see CA-4682
; CHECK: call void @llvm.memmove.p0{{.*}}.p2{{.*}}.i64
; CHECK: call void @llvm.memmove.p2{{.*}}.p1{{.*}}.i32

define void @memmove_kernel_32_p0(i8 *%out, i8 *%in, i32 %size) {
entry:
  call void @llvm.memmove.p0.p0.i32(i8 *%out, i8 *%in, i32 %size, i1 false)
  ret void
}

define void @memmove_kernel_32_p1(i8 addrspace(1)  *%out, i8 addrspace(1) *%in, i32 %size) {
entry:
  call void @llvm.memmove.p1.p1.i32(i8 addrspace(1)  *%out, i8 addrspace(1) *%in, i32 %size, i1 false)
  ret void
}

define void @memmove_kernel_64_p0_p2(i8 *%out, i8 addrspace(2) *%in, i64 %size) {
entry:
  call void @llvm.memmove.p0.p2.i64(i8 *%out, i8 addrspace(2) *%in, i64 %size, i1 false)
  ret void
}

define void @memmove_kernel_32_p2_p1(i8 addrspace(2) *%out, i8 addrspace(1) *%in, i32 %size) {
entry:
  call void @llvm.memmove.p2.p1.i32(i8 addrspace(2) *%out, i8 addrspace(1) *%in, i32 %size, i1 false)
  ret void
}

declare void @llvm.memmove.p0.p0.i32(i8 * noalias nocapture writeonly, i8 * noalias nocapture readonly, i32, i1 immarg)
declare void @llvm.memmove.p1.p1.i32(i8 addrspace(1) * noalias nocapture writeonly, i8 addrspace(1) * noalias nocapture readonly, i32, i1 immarg)
declare void @llvm.memmove.p0.p2.i64(i8 * noalias nocapture writeonly, i8 addrspace(2) * noalias nocapture readonly, i64, i1 immarg)
declare void @llvm.memmove.p2.p1.i32(i8 addrspace(2) * noalias nocapture writeonly, i8 addrspace(1) * noalias nocapture readonly, i32, i1 immarg)
