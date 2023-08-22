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

; RUN: muxc --passes barriers-pass,verify < %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

define void @lifetime(ptr addrspace(1) %input, ptr addrspace(1) %output) #0 {
entry:
; Check that we've removed all assume-like intrinsics that were using the
; alloca, as they're unsafe to keep around.
; * The llvm.assume may not hold, though admittedly this is a contrived
;   example.
; * It should be self-evident that llvm.lifetime.invariant.* intrinsics no
;   longer hold.
; * For llvm.lifetime.* intrinsics, note that %live_gep_addr is also the 'base'
;   of the live mem info, and so replacing %addr with %live_gep_addr in the
;   lifetime.start intrinsic essentially tells LLVM that *all* memory access
;   *based on* the live mem info also start at the same point which we call
;   lifetime.start. This is not true, as may we wish to access pointers based
;   on the live mem info beforehand, such as the store to %live_gep_b (which is
;   inserted before where the lifetime.start would have been).
; CHECK: define internal i32 @lifetime.mux-barrier-region(
; CHECK:      %live_gep_addr = getelementptr inbounds %lifetime_live_mem_info, ptr %2, i32 0, i32 0
; CHECK-NEXT: %live_gep_b = getelementptr inbounds %lifetime_live_mem_info, ptr %2, i32 0, i32 1
; CHECK-NEXT: %b = load i32, ptr addrspace(1) %0, align 4
; CHECK-NEXT: store i32 %b, ptr %live_gep_b, align 4
; CHECK-NEXT: store i32 4, ptr %live_gep_addr, align 4
; CHECK-NEXT: ret i32
  %addr = alloca i32, align 4
  call void @llvm.assume(i1 true) [ "dereferenceable"(ptr %addr, i64 4) ]
  %p = call ptr @llvm.invariant.start.p0(i64 4, ptr %addr)
  %b = load i32, ptr addrspace(1) %input
  call void @llvm.lifetime.start.p0(i64 4, ptr %addr)
  call void @llvm.invariant.end.p0(ptr %p, i64 4, ptr %addr)
  store i32 4, ptr %addr, align 4

  call void @__mux_work_group_barrier(i32 0, i32 1, i32 272)

; After the barrier, next region: check there's no lifetime.end either
; CHECK: define internal i32 @lifetime.mux-barrier-region.1(
; CHECK-NOT: call void @llvm.lifetime.end
; CHECK: ret i32
  %ld = load i32, ptr %addr, align 4
  call void @llvm.lifetime.end.p0(i64 4, ptr %addr)
  %res = add i32 %ld, %b
  store i32 %res, ptr addrspace(1) %output
  ret void
}

declare void @llvm.assume(i1)

declare void @llvm.lifetime.end.p0(i64, ptr nocapture)
declare void @llvm.lifetime.start.p0(i64, ptr nocapture)

declare ptr @llvm.invariant.start.p0(i64, ptr nocapture)
declare void @llvm.invariant.end.p0(ptr, i64, ptr nocapture)

; Function Attrs: convergent nounwind
declare void @__mux_work_group_barrier(i32, i32, i32)

attributes #0 = { norecurse nounwind "mux-kernel"="entry-point" }
