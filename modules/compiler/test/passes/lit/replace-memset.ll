; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes replace-mem-intrins,verify -S %s -opaque-pointers | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK-NOT: call void @llvm.memset

define void @memset_kernel_64_p0(ptr %out) {
entry:
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 1 dereferenceable(256) %out, i8 0, i64 256, i1 false)
  ret void
}

define void @memset_kernel_32_p0(ptr %out) {
entry:
  tail call void @llvm.memset.p0.i32(ptr noundef nonnull align 1 dereferenceable(256) %out, i8 0, i32 256, i1 false)
  ret void
}

define void @memset_kernel_64_p1(ptr addrspace(1) %out) {
entry:
  tail call void @llvm.memset.p1.i64(ptr addrspace(1) noundef nonnull align 1 dereferenceable(256) %out, i8 0, i64 256, i1 false)
  ret void
}

define void @memset_kernel_32_p1(ptr addrspace(1) %out) {
entry:
  tail call void @llvm.memset.p1.i32(ptr addrspace(1) noundef nonnull align 1 dereferenceable(256) %out, i8 0, i32 256, i1 false)
  ret void
}

define void @memset_kernel_64_p2(ptr addrspace(2) %out) {
entry:
  tail call void @llvm.memset.p2.i64(ptr addrspace(2) noundef nonnull align 1 dereferenceable(256) %out, i8 0, i64 256, i1 false)
  ret void
}

define void @memset_kernel_32_p2(ptr addrspace(2) %out) {
entry:
  tail call void @llvm.memset.p2.i32(ptr addrspace(2) noundef nonnull align 1 dereferenceable(256) %out, i8 0, i32 256, i1 false)
  ret void
}

; Function Attrs: argmemonly mustprogress nocallback nofree nounwind willreturn writeonly
declare void @llvm.memset.p0.i64(ptr nocapture writeonly, i8, i64, i1 immarg)

; Function Attrs: argmemonly mustprogress nocallback nofree nounwind willreturn writeonly
declare void @llvm.memset.p0.i32(ptr nocapture writeonly, i8, i32, i1 immarg)

; Function Attrs: argmemonly mustprogress nocallback nofree nounwind willreturn writeonly
declare void @llvm.memset.p1.i64(ptr addrspace(1) nocapture writeonly, i8, i64, i1 immarg)

; Function Attrs: argmemonly mustprogress nocallback nofree nounwind willreturn writeonly
declare void @llvm.memset.p1.i32(ptr addrspace(1) nocapture writeonly, i8, i32, i1 immarg)

; Function Attrs: argmemonly mustprogress nocallback nofree nounwind willreturn writeonly
declare void @llvm.memset.p2.i64(ptr addrspace(2) nocapture writeonly, i8, i64, i1 immarg)

; Function Attrs: argmemonly mustprogress nocallback nofree nounwind willreturn writeonly
declare void @llvm.memset.p2.i32(ptr addrspace(2) nocapture writeonly, i8, i32, i1 immarg)
