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
  %add4 = add nsw i32 %mul3, 3
  %idxprom = sext i32 %add4 to i64
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %in, i64 %idxprom
  %0 = load i8, i8 addrspace(1)* %arrayidx, align 1
  %shl = shl i8 %0, 1
  %add10 = add nsw i32 %mul3, 2
  %idxprom11 = sext i32 %add10 to i64
  %arrayidx12 = getelementptr inbounds i8, i8 addrspace(1)* %in, i64 %idxprom11
  %1 = load i8, i8 addrspace(1)* %arrayidx12, align 1
  %sub = sub i8 %shl, %1
  %idxprom18 = sext i32 %add to i64
  %arrayidx19 = getelementptr inbounds i8, i8 addrspace(1)* %out, i64 %idxprom18
  store i8 %sub, i8 addrspace(1)* %arrayidx19, align 1
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; CHECK: spir_kernel void @load16
; CHECK: load <4 x i8>
; CHECK: load <4 x i8>
; CHECK-NOT: load <4 x i8>
; CHECK-NOT: call <4 x i8> @__vecz_b_interleaved_load
; CHECK-NOT: call <4 x i8> @__vecz_b_gather_load
; CHECK: shufflevector <4 x i8>
; CHECK: shufflevector <4 x i8>
; CHECK-NOT: shufflevector <4 x i8>
; CHECK: ret void
