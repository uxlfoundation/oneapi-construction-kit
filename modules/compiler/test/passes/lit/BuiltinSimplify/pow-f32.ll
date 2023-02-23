; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes builtin-simplify,verify -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @pow_no_fold(float addrspace(1)* %a, float addrspace(1)* %b) {
entry:
  %0 = load float, float addrspace(1)* %a, align 4
  %1 = load float, float addrspace(1)* %b, align 4
  %call = tail call spir_func float @_Z3powff(float %0, float %1)
  store float %call, float addrspace(1)* %a, align 4
  ret void
}

declare spir_func float @_Z3powff(float, float)

define spir_kernel void @pow_whole_numbers(float addrspace(1)* %f, <2 x float> addrspace(1)* %f2) {
entry:
  %0 = load float, float addrspace(1)* %f, align 4
  %call = tail call spir_func float @_Z3powff(float %0, float 1.000000e+00)
  %call1 = tail call spir_func float @_Z3powff(float %call, float 0x4170000000000000)
  %call2 = tail call spir_func float @_Z3powff(float %call1, float 0x4170000020000000)
  store float %call2, float addrspace(1)* %f, align 4
  %1 = load <2 x float>, <2 x float> addrspace(1)* %f2, align 8
  %call3 = tail call spir_func <2 x float> @_Z3powDv2_fS_(<2 x float> %1, <2 x float> <float 1.000000e+00, float 2.000000e+00>)
  %call5 = tail call spir_func <2 x float> @_Z3powDv2_fS_(<2 x float> %call3, <2 x float> <float 1.000000e+00, float 0x4170000000000000>)
  %call7 = tail call spir_func <2 x float> @_Z3powDv2_fS_(<2 x float> %call5, <2 x float> <float 1.000000e+00, float 0x4170000020000000>)
  store <2 x float> %call7, <2 x float> addrspace(1)* %f2, align 8
  ret void
}

declare spir_func <2 x float> @_Z3powDv2_fS_(<2 x float>, <2 x float>)

define spir_kernel void @pow_recip_whole_numbers(float addrspace(1)* %f, <2 x float> addrspace(1)* %f2) {
entry:
  %0 = load float, float addrspace(1)* %f, align 4
  %call = tail call spir_func float @_Z3powff(float %0, float 5.000000e-01)
  %call1 = tail call spir_func float @_Z3powff(float %call, float 0x3E70000000000000)
  %call2 = tail call spir_func float @_Z3powff(float %call1, float 0x3E6FFFFFC0000000)
  store float %call2, float addrspace(1)* %f, align 4
  %1 = load <2 x float>, <2 x float> addrspace(1)* %f2, align 8
  %call3 = tail call spir_func <2 x float> @_Z3powDv2_fS_(<2 x float> %1, <2 x float> <float 1.000000e+00, float 5.000000e-01>)
  %call5 = tail call spir_func <2 x float> @_Z3powDv2_fS_(<2 x float> %call3, <2 x float> <float 1.000000e+00, float 0x3E70000000000000>)
  %call7 = tail call spir_func <2 x float> @_Z3powDv2_fS_(<2 x float> %call5, <2 x float> <float 1.000000e+00, float 0x3E6FFFFFC0000000>)
  store <2 x float> %call7, <2 x float> addrspace(1)* %f2, align 8
  ret void
}

define spir_kernel void @powr_no_fold(float addrspace(1)* %a, float addrspace(1)* %b) {
entry:
  %0 = load float, float addrspace(1)* %a, align 4
  %1 = load float, float addrspace(1)* %b, align 4
  %call = tail call spir_func float @_Z4powrff(float %0, float %1)
  store float %call, float addrspace(1)* %a, align 4
  ret void
}

declare spir_func float @_Z4powrff(float, float)

define spir_kernel void @powr_whole_numbers(float addrspace(1)* %f, <2 x float> addrspace(1)* %f2) {
entry:
  %0 = load float, float addrspace(1)* %f, align 4
  %call = tail call spir_func float @_Z4powrff(float %0, float 1.000000e+00)
  %call1 = tail call spir_func float @_Z4powrff(float %call, float 0x4170000000000000)
  %call2 = tail call spir_func float @_Z4powrff(float %call1, float 0x4170000020000000)
  store float %call2, float addrspace(1)* %f, align 4
  %1 = load <2 x float>, <2 x float> addrspace(1)* %f2, align 8
  %call3 = tail call spir_func <2 x float> @_Z4powrDv2_fS_(<2 x float> %1, <2 x float> <float 1.000000e+00, float 2.000000e+00>)
  %call5 = tail call spir_func <2 x float> @_Z4powrDv2_fS_(<2 x float> %call3, <2 x float> <float 1.000000e+00, float 0x4170000000000000>)
  %call7 = tail call spir_func <2 x float> @_Z4powrDv2_fS_(<2 x float> %call5, <2 x float> <float 1.000000e+00, float 0x4170000020000000>)
  store <2 x float> %call7, <2 x float> addrspace(1)* %f2, align 8
  ret void
}

declare spir_func <2 x float> @_Z4powrDv2_fS_(<2 x float>, <2 x float>)

; CHECK: define {{(dso_local )?}}spir_kernel void @pow_no_fold
; CHECK: call spir_func float @_Z3powff
; CHECK: define {{(dso_local )?}}spir_kernel void @pow_whole_numbers
; CHECK: call spir_func float @_Z4pownfi
; CHECK: call spir_func float @_Z4pownfi
; CHECK: call spir_func float @_Z3powff
; CHECK: call spir_func <2 x float> @_Z4pownDv2_fDv2_i
; CHECK: call spir_func <2 x float> @_Z4pownDv2_fDv2_i
; CHECK: call spir_func <2 x float> @_Z3powDv2_fS_
; CHECK: define {{(dso_local )?}}spir_kernel void @pow_recip_whole_numbers
; CHECK: call spir_func float @_Z5rootnfi
; CHECK: call spir_func float @_Z5rootnfi
; CHECK: call spir_func float @_Z3powff
; CHECK: call spir_func <2 x float> @_Z5rootnDv2_fDv2_i
; CHECK: call spir_func <2 x float> @_Z5rootnDv2_fDv2_i
; CHECK: call spir_func <2 x float> @_Z3powDv2_fS_
; CHECK: define {{(dso_local )?}}spir_kernel void @powr_no_fold
; CHECK: call spir_func float @_Z4powrff
; CHECK: define {{(dso_local )?}}spir_kernel void @powr_whole_numbers
; CHECK: call spir_func float @_Z4pownfi
; CHECK: call spir_func float @_Z4pownfi
; CHECK: call spir_func float @_Z4powrff
; CHECK: call spir_func <2 x float> @_Z4pownDv2_fDv2_i
; CHECK: call spir_func <2 x float> @_Z4pownDv2_fDv2_i
; CHECK: call spir_func <2 x float> @_Z4powrDv2_fS_
