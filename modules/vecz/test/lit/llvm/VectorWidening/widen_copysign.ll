; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; REQUIRES: llvm-12+
; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)

declare float @llvm.copysign.f32(float, float)
declare <2 x float> @llvm.copysign.v2f32(<2 x float>, <2 x float>)

define spir_kernel void @copysignff(float* %pa, float* %pb, float* %pc) {
entry:
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %a = getelementptr float, float* %pa, i64 %idx
  %b = getelementptr float, float* %pb, i64 %idx
  %c = getelementptr float, float* %pc, i64 %idx
  %la = load float, float* %a, align 16
  %lb = load float, float* %b, align 16
  %res = call float @llvm.copysign.f32(float %la, float %lb)
  store float %res, float* %c, align 16
  ret void
}

define spir_kernel void @copysignvf(<2 x float>* %pa, <2 x float>* %pb, <2 x float>* %pc) {
entry:
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %a = getelementptr <2 x float>, <2 x float>* %pa, i64 %idx
  %b = getelementptr <2 x float>, <2 x float>* %pb, i64 %idx
  %c = getelementptr <2 x float>, <2 x float>* %pc, i64 %idx
  %la = load <2 x float>, <2 x float>* %a, align 16
  %lb = load <2 x float>, <2 x float>* %b, align 16
  %res = call <2 x float> @llvm.copysign.v2f32(<2 x float> %la, <2 x float> %lb)
  store <2 x float> %res, <2 x float>* %c, align 16
  ret void
}

; CHECK-GE15: define spir_kernel void @__vecz_v4_copysignff(ptr %pa, ptr %pb, ptr %pc)
; CHECK-LT15: define spir_kernel void @__vecz_v4_copysignff(float* %pa, float* %pb, float* %pc)
; CHECK: entry:
; CHECK: %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
; CHECK-GE15: %a = getelementptr float, ptr %pa, i64 %idx
; CHECK-LT15: %a = getelementptr float, float* %pa, i64 %idx
; CHECK-GE15: %b = getelementptr float, ptr %pb, i64 %idx
; CHECK-LT15: %b = getelementptr float, float* %pb, i64 %idx
; CHECK-GE15: %c = getelementptr float, ptr %pc, i64 %idx
; CHECK-LT15: %c = getelementptr float, float* %pc, i64 %idx
; CHECK-LT15: %0 = bitcast float* %a to <4 x float>*
; CHECK-GE15: [[T0:%.*]] = load <4 x float>, ptr %a, align 4
; CHECK-LT15: [[T0:%.*]] = load <4 x float>, <4 x float>* %0, align 4
; CHECK-LT15: %2 = bitcast float* %b to <4 x float>*
; CHECK-GE15: [[T1:%.*]] = load <4 x float>, ptr %b, align 4
; CHECK-LT15: [[T1:%.*]] = load <4 x float>, <4 x float>* %2, align 4
; CHECK: %res1 = call <4 x float> @llvm.copysign.v4f32(<4 x float> [[T0]], <4 x float> [[T1]])
; CHECK-GE15: store <4 x float> %res1, ptr %c, align 4
; CHECK-LT15: %4 = bitcast float* %c to <4 x float>*
; CHECK-LT15: store <4 x float> %res1, <4 x float>* %4, align 4
; CHECK: ret void

; CHECK-GE15: define spir_kernel void @__vecz_v4_copysignvf(ptr %pa, ptr %pb, ptr %pc)
; CHECK-LT15: define spir_kernel void @__vecz_v4_copysignvf(<2 x float>* %pa, <2 x float>* %pb, <2 x float>* %pc)
; CHECK: entry:
; CHECK: %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
; CHECK-GE15: %a = getelementptr <2 x float>, ptr %pa, i64 %idx
; CHECK-LT15: %a = getelementptr <2 x float>, <2 x float>* %pa, i64 %idx
; CHECK-GE15: %b = getelementptr <2 x float>, ptr %pb, i64 %idx
; CHECK-LT15: %b = getelementptr <2 x float>, <2 x float>* %pb, i64 %idx
; CHECK-GE15: %c = getelementptr <2 x float>, ptr %pc, i64 %idx
; CHECK-LT15: %c = getelementptr <2 x float>, <2 x float>* %pc, i64 %idx
; CHECK-LT15: %0 = bitcast <2 x float>* %a to <8 x float>*
; CHECK-GE15: [[T0:%.*]] = load <8 x float>, ptr %a, align 4
; CHECK-LT15: [[T0:%.*]] = load <8 x float>, <8 x float>* %0, align 4
; CHECK-LT15: %2 = bitcast <2 x float>* %b to <8 x float>*
; CHECK-GE15: [[T1:%.*]] = load <8 x float>, ptr %b, align 4
; CHECK-LT15: [[T1:%.*]] = load <8 x float>, <8 x float>* %2, align 4
; CHECK: %res1 = call <8 x float> @llvm.copysign.v8f32(<8 x float> [[T0]], <8 x float> [[T1]])
; CHECK-LT15: %4 = bitcast <2 x float>* %c to <8 x float>*
; CHECK-GE15: store <8 x float> %res1, ptr %c, align 8
; CHECK-LT15: store <8 x float> %res1, <8 x float>* %4, align 8
; CHECK: ret void
