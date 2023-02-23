; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes replace-mem-intrins,verify -S %s | %filecheck %s

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
