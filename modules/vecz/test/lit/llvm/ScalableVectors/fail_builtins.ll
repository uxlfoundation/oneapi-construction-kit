; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %not %veczc -k fail_builtins -vecz-scalable -vecz-simd-width=4 -S < %s 2>&1 | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @fail_builtins(float* %aptr, float* %zptr) {
entry:
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidxa = getelementptr inbounds float, float* %aptr, i64 %idx
  %arrayidxz = getelementptr inbounds float, float* %zptr, i64 %idx
  %a = load float, float* %arrayidxa, align 4
  %math = call spir_func float @_Z4tanff(float %a)
  store float %math, float* %arrayidxz, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)
declare spir_func float @_Z4tanff(float)

; We can't scalarize this builtin call
; CHECK: Error: Failed to vectorize function 'fail_builtins'
