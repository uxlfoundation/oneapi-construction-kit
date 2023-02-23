; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %llvm-as -o %t.in.bc %s
; RUN: %veczc -k CSMain -vecz-simd-width=8 -o %t.out.bc %t.in.bc
; RUN: %llvm-dis -o %t %t.out.bc
; RUN: %filecheck < %t %s

; We explicitly set the pointer size to 64 bits to catch any invalid sign
; extensions resulting from offset info analysis.

target datalayout = "e-m:e-p:64:32-i1:32-i8:32-i16:32-i32:32-i64:64-f16:32-f32:32-f:64:64-n8:16:32:64"
target triple = "dxil-ms-dx"

define void @CSMain(i32 addrspace(1)* noalias %offsets, i8 addrspace(1)* noalias %in, i8 addrspace(1)* noalias %out) {
entry:
  %tid.0 = call i32 @dx.op.threadId.i32(i32 93, i32 0)
  %tid = add i32 %tid.0, 2
  %ptr_offset = getelementptr inbounds i32, i32 addrspace(1)* %offsets, i32 %tid
  %offset = load i32, i32 addrspace(1)* %ptr_offset, align 1
  %idx.0 = add i32 %tid, %offset
  %idx = add i32 %idx.0, 2
  %ptr_in = getelementptr inbounds i8, i8 addrspace(1)* %in, i32 %idx
  %val = load i8, i8 addrspace(1)* %ptr_in, align 1
  %ptr_out = getelementptr inbounds i8, i8 addrspace(1)* %out, i32 %idx
  store i8 %val, i8 addrspace(1)* %ptr_out, align 1
  ret void
}

; Function Attrs: nounwind readnone
declare i32 @dx.op.threadId.i32(i32, i32) #0

; CHECK: define void @__vecz_v8_CSMain
; CHECK: call i32 @dx.op.threadId.i32
; CHECK-NEXT: insertelement <8 x i32>
; CHECK-NEXT: %[[SPLAT:.+]] = shufflevector <8 x i32>
; CHECK-NEXT: add <8 x i32> %[[SPLAT]], <i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9>
