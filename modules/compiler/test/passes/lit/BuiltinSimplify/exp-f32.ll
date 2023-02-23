; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes builtin-simplify,verify -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @exp_no_fold(float addrspace(1)* %a) {
entry:
  %0 = load float, float addrspace(1)* %a, align 4
  %call = tail call spir_func float @_Z3expf(float %0)
  store float %call, float addrspace(1)* %a, align 4
  ret void
}

declare spir_func float @_Z3expf(float)

define spir_kernel void @exp_fold(float addrspace(1)* %f, <2 x float> addrspace(1)* %f2) {
entry:
  %call = tail call spir_func float @_Z3expf(float 1.000000e+00)
  store float %call, float addrspace(1)* %f, align 4
  %call1 = tail call spir_func <2 x float> @_Z3expDv2_f(<2 x float> <float 1.000000e+00, float 4.200000e+01>)
  store <2 x float> %call1, <2 x float> addrspace(1)* %f2, align 8
  ret void
}

declare spir_func <2 x float> @_Z3expDv2_f(<2 x float>)

define spir_kernel void @half_exp_no_fold(float addrspace(1)* %a) {
entry:
  %0 = load float, float addrspace(1)* %a, align 4
  %call = tail call spir_func float @_Z8half_expf(float %0)
  store float %call, float addrspace(1)* %a, align 4
  ret void
}

declare spir_func float @_Z8half_expf(float)

define spir_kernel void @half_exp_fold(float addrspace(1)* %f, <2 x float> addrspace(1)* %f2) {
entry:
  %call = tail call spir_func float @_Z8half_expf(float 1.000000e+00)
  store float %call, float addrspace(1)* %f, align 4
  %call1 = tail call spir_func <2 x float> @_Z8half_expDv2_f(<2 x float> <float 1.000000e+00, float 4.200000e+01>)
  store <2 x float> %call1, <2 x float> addrspace(1)* %f2, align 8
  ret void
}

declare spir_func <2 x float> @_Z8half_expDv2_f(<2 x float>)

define spir_kernel void @native_exp_no_fold(float addrspace(1)* %a) {
entry:
  %0 = load float, float addrspace(1)* %a, align 4
  %call = tail call spir_func float @_Z10native_expf(float %0)
  store float %call, float addrspace(1)* %a, align 4
  ret void
}

declare spir_func float @_Z10native_expf(float)

define spir_kernel void @native_exp_fold(float addrspace(1)* %f, <2 x float> addrspace(1)* %f2) {
entry:
  %call = tail call spir_func float @_Z10native_expf(float 1.000000e+00)
  store float %call, float addrspace(1)* %f, align 4
  %call1 = tail call spir_func <2 x float> @_Z10native_expDv2_f(<2 x float> <float 1.000000e+00, float 4.200000e+01>)
  store <2 x float> %call1, <2 x float> addrspace(1)* %f2, align 8
  ret void
}

declare spir_func <2 x float> @_Z10native_expDv2_f(<2 x float>)

define spir_kernel void @exp_double_call(float addrspace(1)* %f) {
entry:
  %call = tail call spir_func float @_Z3expf(float 1.000000e+00)
  %call1 = tail call spir_func float @_Z3expf(float %call)
  store float %call1, float addrspace(1)* %f, align 4
  ret void
}

; CHECK: define {{(dso_local )?}}spir_kernel void @exp_no_fold
; CHECK: call spir_func float @_Z3expf
; CHECK: define {{(dso_local )?}}spir_kernel void @exp_fold
; CHECK-NOT: call spir_func float @_Z3expf
; CHECK-NOT: call spir_func <2 x float> @_Z3expDv2_f
; CHECK: define {{(dso_local )?}}spir_kernel void @half_exp_no_fold
; CHECK: call spir_func float @_Z8half_expf
; CHECK: define {{(dso_local )?}}spir_kernel void @half_exp_fold
; CHECK-NOT: call spir_func float @_Z8half_expf
; CHECK-NOT: call spir_func <2 x float> @_Z8half_expDv2_f
; CHECK: define {{(dso_local )?}}spir_kernel void @native_exp_no_fold
; CHECK: call spir_func float @_Z10native_expf
; CHECK: define {{(dso_local )?}}spir_kernel void @native_exp_fold
; CHECK-NOT: call spir_func float @_Z10native_expf
; CHECK-NOT: call spir_func <2 x float> @_Z10native_expDv2_f
; CHECK: define {{(dso_local )?}}spir_kernel void @exp_double_call
; CHECK-NOT: call spir_func float @_Z3expf
