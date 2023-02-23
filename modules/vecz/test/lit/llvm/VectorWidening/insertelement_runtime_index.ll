; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k runtime_index -vecz-simd-width=4 -vecz-passes=packetizer -vecz-choices=TargetIndependentPacketization -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @runtime_index(<4 x i32>* %in, <4 x i32>* %out, i32* %index) {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32>* %in, i64 %call
  %0 = load <4 x i32>, <4 x i32>* %arrayidx
  %arrayidx1 = getelementptr inbounds <4 x i32>, <4 x i32>* %out, i64 %call
  store <4 x i32> %0, <4 x i32>* %arrayidx1
  %arrayidx2 = getelementptr inbounds i32, i32* %index, i64 %call
  %1 = load i32, i32* %arrayidx2
  %arrayidx3 = getelementptr inbounds <4 x i32>, <4 x i32>* %out, i64 %call
  %vecins = insertelement <4 x i32> %0, i32 42, i32 %1
  store <4 x i32> %vecins, <4 x i32>* %arrayidx3
  ret void
}

; CHECK: define spir_kernel void @__vecz_v4_runtime_index

; CHECK-GE15: %[[INTO:.+]]  = load <16 x i32>, ptr %arrayidx, align 4
; CHECK-LT15: %[[INTO:.+]]  = load <16 x i32>, <16 x i32>* %0, align 4
; CHECK-GE15: %[[LD:.+]] = load <4 x i32>, ptr
; CHECK-LT15: %[[LD:.+]] = load <4 x i32>, <4 x i32>*
; CHECK: %[[ADD:.+]] = add <4 x i32> %[[LD]], <i32 0, i32 4, i32 8, i32 12>

; The inserts got widened
; CHECK: %[[ELT0:.+]] = extractelement <4 x i32> %[[ADD]], i32 0
; CHECK: %[[INS0:.+1]] = insertelement <16 x i32> %[[INTO]], i32 42, i32 %[[ELT0]]
; CHECK: %[[ELT1:.+]] = extractelement <4 x i32> %[[ADD]], i32 1
; CHECK: %[[INS1:.+]] = insertelement <16 x i32> %[[INS0]], i32 42, i32 %[[ELT1]]
; CHECK: %[[ELT2:.+]] = extractelement <4 x i32> %[[ADD]], i32 2
; CHECK: %[[INS2:.+]] = insertelement <16 x i32> %[[INS1]], i32 42, i32 %[[ELT2]]
; CHECK: %[[ELT3:.+]] = extractelement <4 x i32> %[[ADD]], i32 3
; CHECK: %[[INS3:.+]] = insertelement <16 x i32> %[[INS2]], i32 42, i32 %[[ELT3]]

; No shuffles..
; CHECK-NOT: shufflevector

; One widened store directly storing the result
; CHECK: store <16 x i32> %[[INS3]]
; CHECK: ret void
