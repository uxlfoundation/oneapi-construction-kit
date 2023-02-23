; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -k cast -vecz-scalable -S < %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @cast(i32* %aptr, float* %zptr) {
entry:
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidxa = getelementptr inbounds i32, i32* %aptr, i64 %idx
  %arrayidxz = getelementptr inbounds float, float* %zptr, i64 %idx
  %a = load i32, i32* %arrayidxa, align 4
  %c = sitofp i32 %a to float
  store float %c, float* %arrayidxz, align 4
  ret void
}

; Check that passing -vecz-scalable with no width automatically chooses an
; appropriate scalable vectorization factor.
; CHECK: define spir_kernel void @__vecz_nxv[[VF:[0-9]+]]_cast
; CHECK: sitofp <vscale x [[VF]] x i32> {{%[0-9]+}} to <vscale x [[VF]] x float>
declare spir_func i64 @_Z13get_global_idj(i32)
