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

; RUN: %muxc --passes replace-mem-intrins,verify -S %s  | %filecheck %s

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
