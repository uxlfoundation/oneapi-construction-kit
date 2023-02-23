; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k extract_runtime_index -vecz-simd-width=4 -vecz-passes=packetizer -vecz-choices=TargetIndependentPacketization -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32) #1

; Function Attrs: nounwind
define spir_kernel void @extract_runtime_index(i32 addrspace(1)* %in, <4 x i8> %x, i8 addrspace(1)* %out) #0 {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0) #2
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 %call
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %vecext = extractelement <4 x i8> %x, i32 %0
  %arrayidx1 = getelementptr inbounds i8, i8 addrspace(1)* %out, i64 %call
  store i8 %vecext, i8 addrspace(1)* %arrayidx1, align 1
  ret void
}

; CHECK: define spir_kernel void @__vecz_v4_extract_runtime_index
; CHECK-GE15: %[[LD:.+]] = load <4 x i32>, ptr addrspace(1) %
; CHECK-LT15: %[[LD:.+]] = load <4 x i32>, <4 x i32> addrspace(1)* %

; No splitting of the widened source vector
; CHECK-NOT: shufflevector

; Extract directly from the uniform source with vectorized indices and insert directly into result
; CHECK: %[[IND0:.+]] = extractelement <4 x i32> %[[LD]], i32 0
; CHECK: %[[EXT0:.+]] = extractelement <4 x i8> %x, i32 %[[IND0]]
; CHECK: %[[INS0:.+]] = insertelement <4 x i8> undef, i8 %[[EXT0]], i32 0
; CHECK: %[[IND1:.+]] = extractelement <4 x i32> %[[LD]], i32 1
; CHECK: %[[EXT1:.+]] = extractelement <4 x i8> %x, i32 %[[IND1]]
; CHECK: %[[INS1:.+]] = insertelement <4 x i8> %[[INS0]], i8 %[[EXT1]], i32 1
; CHECK: %[[IND2:.+]] = extractelement <4 x i32> %[[LD]], i32 2
; CHECK: %[[EXT2:.+]] = extractelement <4 x i8> %x, i32 %[[IND2]]
; CHECK: %[[INS2:.+]] = insertelement <4 x i8> %[[INS1]], i8 %[[EXT2]], i32 2
; CHECK: %[[IND3:.+]] = extractelement <4 x i32> %[[LD]], i32 3
; CHECK: %[[EXT3:.+]] = extractelement <4 x i8> %x, i32 %[[IND3]]
; CHECK: %[[INS3:.+]] = insertelement <4 x i8> %[[INS2]], i8 %[[EXT3]], i32 3
; CHECK-GE15: store <4 x i8> %[[INS3]], ptr addrspace(1) %{{.+}}, align 1
; CHECK-LT15: store <4 x i8> %[[INS3]], <4 x i8> addrspace(1)* %{{.+}}, align 1
; CHECK: ret void
