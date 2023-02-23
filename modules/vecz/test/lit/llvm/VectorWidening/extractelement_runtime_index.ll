; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k extract_runtime_index -vecz-simd-width=4 -vecz-passes=packetizer -vecz-choices=TargetIndependentPacketization -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32) #1

; Function Attrs: nounwind
define spir_kernel void @extract_runtime_index(<4 x float> addrspace(1)* %in, i32 %x, float addrspace(1)* %out) #0 {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0) #2
  %arrayidx = getelementptr inbounds <4 x float>, <4 x float> addrspace(1)* %in, i64 %call
  %0 = load <4 x float>, <4 x float> addrspace(1)* %arrayidx, align 4
  %vecext = extractelement <4 x float> %0, i32 %x
  %arrayidx1 = getelementptr inbounds float, float addrspace(1)* %out, i64 %call
  store float %vecext, float addrspace(1)* %arrayidx1, align 4
  ret void
}

; CHECK: define spir_kernel void @__vecz_v4_extract_runtime_index
; CHECK-GE15: %[[LD:.+]] = load <16 x float>, ptr addrspace(1) %
; CHECK-LT15: %[[LD:.+]] = load <16 x float>, <16 x float> addrspace(1)* %

; No splitting of the widened source vector
; CHECK-NOT: shufflevector

; Extract directly from the widened source and insert directly into result
; CHECK: %[[EXT0:.+]] = extractelement <16 x float> %[[LD]], i32 %x
; CHECK: %[[INS0:.+]] = insertelement <4 x float> undef, float %[[EXT0]], i32 0
; CHECK: %[[IDX1:.+]] = add i32 %x, 4
; CHECK: %[[EXT1:.+]] = extractelement <16 x float> %[[LD]], i32 %[[IDX1]]
; CHECK: %[[INS1:.+]] = insertelement <4 x float> %[[INS0]], float %[[EXT1]], i32 1
; CHECK: %[[IDX2:.+]] = add i32 %x, 8
; CHECK: %[[EXT2:.+]] = extractelement <16 x float> %[[LD]], i32 %[[IDX2]]
; CHECK: %[[INS2:.+]] = insertelement <4 x float> %[[INS1]], float %[[EXT2]], i32 2
; CHECK: %[[IDX3:.+]] = add i32 %x, 12
; CHECK: %[[EXT3:.+]] = extractelement <16 x float> %[[LD]], i32 %[[IDX3]]
; CHECK: %[[INS3:.+]] = insertelement <4 x float> %[[INS2]], float %[[EXT3]], i32 3
; CHECK-GE15: store <4 x float> %[[INS3]], ptr addrspace(1) %{{.+}}
; CHECK-LT15: store <4 x float> %[[INS3]], <4 x float> addrspace(1)* %{{.+}}
; CHECK: ret void
