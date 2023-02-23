; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -vecz-passes=builtin-inlining -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @fmaxff(float %a, float %b, float* %c) {
entry:
  %max = call spir_func float @_Z4fmaxff(float %a, float %b)
  store float %max, float* %c, align 4
  ret void
}

define spir_kernel void @fmaxvf(<2 x float> %a, float %b, <2 x float>* %c) {
entry:
  %max = call spir_func <2 x float> @_Z4fmaxDv2_ff(<2 x float> %a, float %b)
  store <2 x float> %max, <2 x float>* %c, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

declare spir_func float @_Z4fmaxff(float, float)
declare spir_func <2 x float> @_Z4fmaxDv2_ff(<2 x float>, float)

; CHECK-GE15: define spir_kernel void @__vecz_v4_fmaxff(float %a, float %b, ptr %c)
; CHECK-LT15: define spir_kernel void @__vecz_v4_fmaxff(float %a, float %b, float* %c)
; CHECK: entry:
; CHECK: %0 = call float @llvm.maxnum.f32(float %a, float %b)
; CHECK-GE15: store float %0, ptr %c, align 4
; CHECK-LT15: store float %0, float* %c, align 4
; CHECK: ret void

; CHECK-GE15: define spir_kernel void @__vecz_v4_fmaxvf(<2 x float> %a, float %b, ptr %c)
; CHECK-LT15: define spir_kernel void @__vecz_v4_fmaxvf(<2 x float> %a, float %b, <2 x float>* %c)
; CHECK: entry:
; CHECK: %.splatinsert = insertelement <2 x float> {{.*}}, float %b, {{(i32|i64)}} 0
; CHECK: %.splat = shufflevector <2 x float> %.splatinsert, <2 x float> {{.*}}, <2 x i32> zeroinitializer
; CHECK: %0 = call <2 x float> @llvm.maxnum.v2f32(<2 x float> %a, <2 x float> %.splat)
; CHECK-GE15: store <2 x float> %0, ptr %c, align 4
; CHECK-LT15: store <2 x float> %0, <2 x float>* %c, align 4
; CHECK: ret void
