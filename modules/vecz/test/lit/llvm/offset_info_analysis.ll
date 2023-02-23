; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k offset_info_analysis -vecz-passes=packetizer -vecz-simd-width=4 -S < %s | %filecheck %t

; ModuleID = 'Unknown buffer'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @offset_info_analysis(i8 addrspace(1)* noalias %in, i8 addrspace(1)* noalias %out, i32 %width) #0 {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0) #2
  %conv = trunc i64 %call to i32
  %call1 = call spir_func i64 @_Z13get_global_idj(i32 1) #2
  %conv2 = trunc i64 %call1 to i32
  %mul = mul nsw i32 %conv2, %width
  %0 = xor i32 %width, -1
  %add = add i32 %conv, %0
  %add5 = add i32 %add, %mul
  %idxprom = sext i32 %add5 to i64
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %in, i64 %idxprom
  %1 = load i8, i8 addrspace(1)* %arrayidx, align 1
  %mul10 = mul nsw i32 %conv2, %width
  %add11 = add nsw i32 %mul10, %conv
  %idxprom15 = sext i32 %add11 to i64
  %arrayidx16 = getelementptr inbounds i8, i8 addrspace(1)* %out, i64 %idxprom15
  store i8 %1, i8 addrspace(1)* %arrayidx16, align 1
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; This test checks that a 'xor' as a binop operand does correctly get analyzed.
; and masked properly
; CHECK: define spir_kernel void @__vecz_v4_offset_info_analysis
; CHECK-GE15: load <4 x i8>, ptr addrspace(1)
; CHECK-LT15: load <4 x i8>, <4 x i8> addrspace(1)*
; CHECK-NOT-GE15: call <4 x i8> @__vecz_b_gather_load_Dv4_hDv4_u3ptrU3AS1
; CHECK-NOT-LT15: call <4 x i8> @__vecz_b_gather_load_Dv4_hDv4_PU3AS1h
; CHECK: ret void

; Check the gather load definition is not generated.
;CHECK-NOT: declare <4 x i8> @__vecz_b_gather_load_Dv4_hDv4
