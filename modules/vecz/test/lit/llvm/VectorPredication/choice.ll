; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; REQUIRES: llvm-13+
; Just check that the VectorPredication choice is valid
; RUN: %veczc -k foo -vecz-simd-width=2 -vecz-choices=VectorPredication -S < %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @foo(float* %aptr, float* %zptr) {
entry:
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidxa = getelementptr inbounds float, float* %aptr, i64 %idx
  %arrayidxz = getelementptr inbounds float, float* %zptr, i64 %idx
  %a = load float, float* %arrayidxa, align 4
  store float %a, float* %arrayidxz, align 4
  ret void
}
