; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes replace-mem-intrins,verify -S %s -opaque-pointers | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK-NOT: call void @llvm.memcpy

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

declare void @llvm.memcpy.p0.p0.i32(ptr noalias nocapture writeonly, ptr noalias nocapture readonly, i32, i1 immarg)
declare void @llvm.memcpy.p1.p1.i32(ptr addrspace(1) noalias nocapture writeonly, ptr addrspace(1) noalias nocapture readonly, i32, i1 immarg)
declare void @llvm.memcpy.p0.p2.i64(ptr noalias nocapture writeonly, ptr addrspace(2) noalias nocapture readonly, i64, i1 immarg)
declare void @llvm.memcpy.p2.p1.i32(ptr addrspace(2) noalias nocapture writeonly, ptr addrspace(1) noalias nocapture readonly, i32, i1 immarg)
