; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -k fadd -vecz-scalable -vecz-simd-width=4 -S < %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @fadd(float* %aptr, float* %bptr, float* %zptr) {
entry:
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidxa = getelementptr inbounds float, float* %aptr, i64 %idx
  %arrayidxb = getelementptr inbounds float, float* %bptr, i64 %idx
  %arrayidxz = getelementptr inbounds float, float* %zptr, i64 %idx
  %a = load float, float* %arrayidxa, align 4
  %b = load float, float* %arrayidxb, align 4
  %sum = fadd float %a, %b
  store float %sum, float* %arrayidxz, align 4
  ret void
}

; CHECK: define spir_kernel void @__vecz_nxv4_fadd
; CHECK: load <vscale x 4 x float>, ptr
; CHECK: load <vscale x 4 x float>, ptr
; CHECK: fadd <vscale x 4 x float>
; CHECK: store <vscale x 4 x float>
declare spir_func i64 @_Z13get_global_idj(i32)
