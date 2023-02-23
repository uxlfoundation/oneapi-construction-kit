; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k fmin_vector_scalar -vecz-simd-width=4 -vecz-choices=TargetIndependentPacketization -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)

; Function Attrs: nounwind readnone
declare spir_func <4 x float> @_Z4fminDv4_ff(<4 x float>, float)

; Note that we have to declare the scalar version, because when we vectorize
; an already-vector builtin, we have to scalarize it first. This is the case
; even for Vector Widening, where we don't actually create a call to the
; scalar version, but we retrieve the wide version via the scalar version,
; so the declaration still needs to exist.

; Function Attrs: inlinehint nounwind readnone
declare spir_func float @_Z4fminff(float, float)

; Function Attrs: inlinehint nounwind readnone
declare spir_func <16 x float> @_Z4fminDv16_fS_(<16 x float>, <16 x float>)

define spir_kernel void @fmin_vector_scalar(<4 x float>* %pa, float* %pb, <4 x float>* %pd) {
entry:
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %a = getelementptr <4 x float>, <4 x float>* %pa, i64 %idx
  %b = getelementptr float, float* %pb, i64 %idx
  %d = getelementptr <4 x float>, <4 x float>* %pd, i64 %idx
  %la = load <4 x float>, <4 x float>* %a, align 16
  %lb = load float, float* %b, align 4
  %res = tail call spir_func <4 x float> @_Z4fminDv4_ff(<4 x float> %la, float %lb)
  store <4 x float> %res, <4 x float>* %d, align 16
  ret void
}


; CHECK-GE15: define spir_kernel void @__vecz_v4_fmin_vector_scalar(ptr %pa, ptr %pb, ptr %pd)
; CHECK-LT15: define spir_kernel void @__vecz_v4_fmin_vector_scalar(<4 x float>* %pa, float* %pb, <4 x float>* %pd)
; CHECK: entry:

; It checks that the fmin builtin gets widened by a factor of 4, while its
; scalar operand is sub-splatted to the required <16 x float>.
; CHECK-GE15: %[[LDA:.+]] = load <16 x float>, ptr %{{.+}}
; CHECK-LT15: %[[LDA:.+]] = load <16 x float>, <16 x float>* %{{.+}}
; CHECK-GE15: %[[LDB:.+]] = load <4 x float>, ptr %{{.+}}
; CHECK-LT15: %[[LDB:.+]] = load <4 x float>, <4 x float>* %{{.+}}
; CHECK: %[[SPL:.+]] = shufflevector <4 x float> %[[LDB]], <4 x float> undef, <16 x i32> <i32 0, i32 0, i32 0, i32 0, i32 1, i32 1, i32 1, i32 1, i32 2, i32 2, i32 2, i32 2, i32 3, i32 3, i32 3, i32 3>
; CHECK: %[[RES:.+]] = call <16 x float> @llvm.minnum.v16f32(<16 x float> %[[LDA]], <16 x float> %[[SPL]])
; CHECK-GE15: store <16 x float> %[[RES]], ptr %{{.+}}
; CHECK-LT15: store <16 x float> %[[RES]], <16 x float>* %{{.+}}

; CHECK: ret void
