; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -k extract_constant_index -vecz-simd-width=4 -vecz-choices=FullScalarization -S < %s | %filecheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

; Function Attrs: nounwind
define spir_kernel void @extract_constant_index(<4 x float> addrspace(1)* %in, i32 %x, float addrspace(1)* %out) #0 {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0) #2
  %arrayidx = getelementptr inbounds <4 x float>, <4 x float> addrspace(1)* %in, i64 %call
  %0 = load <4 x float>, <4 x float> addrspace(1)* %arrayidx, align 4
  %vecext = extractelement <4 x float> %0, i32 0;
  %arrayidx1 = getelementptr inbounds float, float addrspace(1)* %out, i64 %call
  store float %vecext, float addrspace(1)* %arrayidx1, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32) #1

; CHECK: define spir_kernel void @__vecz_v4_extract_constant_index
; CHECK: call <4 x float> @__vecz_b_interleaved_load4_4_Dv4
; CHECK: getelementptr inbounds float
; CHECK: store <4 x float>
; CHECK: ret void
