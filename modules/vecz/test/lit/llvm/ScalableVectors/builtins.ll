; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -k builtins -vecz-scalable -vecz-simd-width=4 -S < %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @builtins(float* %aptr, float* %bptr, i32* %zptr) {
entry:
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidxa = getelementptr inbounds float, float* %aptr, i64 %idx
  %arrayidxb = getelementptr inbounds float, float* %bptr, i64 %idx
  %arrayidxz = getelementptr inbounds i32, i32* %zptr, i64 %idx
  %a = load float, float* %arrayidxa, align 4
  %b = load float, float* %arrayidxb, align 4
  %cmp = call spir_func i32 @_Z9isgreaterff(float %a, float %b)
  store i32 %cmp, i32* %arrayidxz, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)
declare spir_func i32 @_Z9isgreaterff(float, float)

; CHECK: void @__vecz_nxv4_builtins
; CHECK:   = fcmp ogt <vscale x 4 x float> %{{.*}}, %{{.*}}
; CHECK:   = zext <vscale x 4 x i1> %relational2 to <vscale x 4 x i32>
