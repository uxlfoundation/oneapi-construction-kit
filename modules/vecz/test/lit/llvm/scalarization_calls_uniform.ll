; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test_calls -vecz-passes=scalarize -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_calls(<4 x float>* %a, <4 x float>* %b, <4 x i32>* %c, <4 x float>* %d) {
entry:
  %0 = load <4 x float>, <4 x float>* %a, align 16
  %1 = load <4 x float>, <4 x float>* %b, align 16
  %2 = load <4 x i32>, <4 x i32>* %c, align 16
  %call = call spir_func <4 x float> @_Z14convert_float4Dv4_i(<4 x i32> %2)
  %3 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %0, <4 x float> %1, <4 x float> %call)
  store <4 x float> %3, <4 x float>* %d, align 16
  ret void
}

declare spir_func <4 x float> @_Z14convert_float4Dv4_i(<4 x i32>)
declare spir_func float @_Z13convert_floati(i32)
declare <4x float> @llvm.fmuladd.v4f32(<4 x float>, <4 x float>, <4 x float>)

; Checks that this function gets vectorized, although because every instruction is
; uniform, the process of vectorization makes no actual changes whatsoever!
; CHECK-GE15: define spir_kernel void @__vecz_v4_test_calls(ptr %a, ptr %b, ptr %c, ptr %d)
; CHECK-LT15: define spir_kernel void @__vecz_v4_test_calls(<4 x float>* %a, <4 x float>* %b, <4 x i32>* %c, <4 x float>* %d)
; CHECK: entry:
; CHECK-GE15: %[[LA:.+]] = load <4 x float>, ptr %a, align 16
; CHECK-LT15: %[[LA:.+]] = load <4 x float>, <4 x float>* %a, align 16
; CHECK-GE15: %[[LB:.+]] = load <4 x float>, ptr %b, align 16
; CHECK-LT15: %[[LB:.+]] = load <4 x float>, <4 x float>* %b, align 16
; CHECK-GE15: %[[LC:.+]] = load <4 x i32>, ptr %c, align 16
; CHECK-LT15: %[[LC:.+]] = load <4 x i32>, <4 x i32>* %c, align 16
; CHECK: %[[CALL:.+]] = call spir_func <4 x float> @_Z14convert_float4Dv4_i(<4 x i32> %[[LC]])
; CHECK: %[[FMAD:.+]] = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %[[LA]], <4 x float> %[[LB]], <4 x float> %[[CALL]])
; CHECK-GE15: store <4 x float> %[[FMAD]], ptr %d, align 16
; CHECK-LT15: store <4 x float> %[[FMAD]], <4 x float>* %d, align 16
; CHECK: ret void
