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

; RUN: muxc --passes replace-module-scope-vars,verify -S %s  | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"


%class.anon = type { i64, %"class.helper"}
%"class.helper" = type { i64, i64 }

@gen_local_struct = internal addrspace(3) global %class.anon undef, align 8
@llvm.compiler.used = appending global [3 x ptr] [ptr @memcpy, ptr @memset, ptr @memmove], section "llvm.metadata"
@a = internal addrspace(3) global i16 undef, align 2

declare void @llvm.memcpy.p0.p3.i64(ptr noalias nocapture writeonly, ptr addrspace(3) noalias nocapture readonly, i64, i1 immarg)

; CHECK: define internal ptr @memcpy(ptr noalias noundef returned writeonly %dst, ptr noalias nocapture noundef readonly %src, i64 noundef %num)
; Function Attrs: mustprogress nofree norecurse nosync nounwind memory(readwrite, inaccessiblemem: none)
define internal ptr @memcpy(ptr noalias noundef returned writeonly %dst, ptr noalias nocapture noundef readonly %src, i64 noundef %num) local_unnamed_addr #7 {
entry:
  ret ptr %dst
}

; CHECK: define internal ptr @memset(ptr noalias noundef returned writeonly %dst,  i32 noundef %value, i64 noundef %num)
; Function Attrs: mustprogress nofree norecurse nosync nounwind memory(readwrite, inaccessiblemem: none)
define internal ptr @memset(ptr noalias noundef returned writeonly %dst,  i32 noundef %value, i64 noundef %num) local_unnamed_addr #7 {
entry:
  ret ptr %dst
}

; CHECK: define internal ptr @memmove(ptr noalias noundef returned writeonly %dst,  ptr noalias nocapture noundef readonly %src, i64 noundef %num)
; Function Attrs: mustprogress nofree norecurse nosync nounwind memory(readwrite, inaccessiblemem: none)
define internal ptr @memmove(ptr noalias noundef returned writeonly %dst,  ptr noalias nocapture noundef readonly %src, i64 noundef %num) local_unnamed_addr #7 {
entry:
  ret ptr %dst
}

; CHECK-DAG: define spir_kernel void @baz.mux-local-var-wrapper(ptr addrspace(1) readonly %in, ptr byval(i32) %out)
define spir_kernel void @baz(ptr addrspace(1) readonly %in, ptr byval(i32) %out) #0 {
  call void @llvm.memcpy.p0.p3.i64(ptr %out, ptr addrspace(3) @a, i64 32, i1 false)
  %ld = load i16, ptr addrspace(3) @a, align 2
  ret void
}

; CHECK-DAG: define internal void @foo_called(ptr addrspace(1) readonly %in, ptr byval(i32) %out, ptr %0)
define internal void @foo_called(ptr addrspace(1) readonly %in, ptr byval(i32) %out){
  call void @llvm.memcpy.p0.p3.i64(ptr %out, ptr addrspace(3) @a, i64 32, i1 false)
  %ld = load i16, ptr addrspace(3) @a, align 2
  ret void
}

; CHECK-DAG: define internal spir_kernel void @foo_kernel(ptr addrspace(1) readonly %in, ptr byval(i32) %out, ptr %0)
; CHECK-DAG: define spir_kernel void @foo_kernel.mux-local-var-wrapper(ptr addrspace(1) readonly %in, ptr byval(i32) %out)
define spir_kernel void @foo_kernel(ptr addrspace(1) readonly %in, ptr byval(i32) %out) #0 {
  call void @foo_called(ptr addrspace(1) readonly %in, ptr byval(i32) %out)
  ret void
}

; CHECK-DAG: define internal void @dead_function(ptr addrspace(1) readonly %in, ptr byval(i32) %out, ptr %0)
define internal void @dead_function(ptr addrspace(1) readonly %in, ptr byval(i32) %out) {
  call void @foo_called(ptr addrspace(1) readonly %in, ptr byval(i32) %out)
  ret void
}

; CHECK-DAG: define internal void @inbounds_gep(ptr %0, ptr %1)
define internal void @inbounds_gep(ptr %0) {
entry:
  store ptr addrspace(4) addrspacecast (ptr addrspace(3) getelementptr inbounds (%class.anon, ptr addrspace(3) @gen_local_struct, i64 0, i32 1, i32 0) to ptr addrspace(4)), ptr %0, align 8
  ret void
}

attributes #0 = { convergent norecurse nounwind "mux-kernel"="entry-point" }
