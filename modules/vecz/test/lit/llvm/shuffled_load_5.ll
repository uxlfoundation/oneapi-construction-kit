; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -S < %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @load16(i8 addrspace(1)* %out, i8 addrspace(1)* %in, i32 %stride) {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %conv = trunc i64 %call to i32
  %call1 = tail call spir_func i64 @_Z13get_global_idj(i32 1)
  %conv2 = trunc i64 %call1 to i32
  %mul = mul nsw i32 %conv2, %stride
  %add = add nsw i32 %mul, %conv
  %mul3 = shl nsw i32 %add, 1
  %idxprom = sext i32 %mul3 to i64
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %in, i64 %idxprom
  %0 = load i8, i8 addrspace(1)* %arrayidx, align 1
  %add7 = or i32 %mul3, 1
  %idxprom8 = sext i32 %add7 to i64
  %arrayidx9 = getelementptr inbounds i8, i8 addrspace(1)* %in, i64 %idxprom8
  %1 = load i8, i8 addrspace(1)* %arrayidx9, align 1
  %add13 = add nsw i32 %mul3, 2
  %idxprom14 = sext i32 %add13 to i64
  %arrayidx15 = getelementptr inbounds i8, i8 addrspace(1)* %in, i64 %idxprom14
  %2 = load i8, i8 addrspace(1)* %arrayidx15, align 1
  %add19 = add nsw i32 %mul3, 3
  %idxprom20 = sext i32 %add19 to i64
  %arrayidx21 = getelementptr inbounds i8, i8 addrspace(1)* %in, i64 %idxprom20
  %3 = load i8, i8 addrspace(1)* %arrayidx21, align 1
  %add24 = add i8 %1, %0
  %add26 = add i8 %add24, %2
  %add28 = add i8 %add26, %3
  %idxprom32 = sext i32 %add to i64
  %arrayidx33 = getelementptr inbounds i8, i8 addrspace(1)* %out, i64 %idxprom32
  store i8 %add28, i8 addrspace(1)* %arrayidx33, align 1
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; CHECK: spir_kernel void @load16
; CHECK: load <4 x i8>
; CHECK: load <4 x i8>
; CHECK: shufflevector <4 x i8>
; CHECK: shufflevector <4 x i8>
; CHECK: load <4 x i8>
; CHECK: load <4 x i8>
; CHECK: shufflevector <4 x i8>
; CHECK: shufflevector <4 x i8>
; CHECK-NOT: load <4 x i8>
; CHECK-NOT: call <4 x i8> @__vecz_b_interleaved_load
; CHECK-NOT: call <4 x i8> @__vecz_b_gather_load
; CHECK-NOT: shufflevector <4 x i8>
; CHECK: ret void
