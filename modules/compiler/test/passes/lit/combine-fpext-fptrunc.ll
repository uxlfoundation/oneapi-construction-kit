; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes "require<device-info>,combine-fpext-fptrunc,verify" -S %s | %filecheck %s --check-prefixes CHECK,DOUBLE
; RUN: %muxc --passes "require<device-info>,combine-fpext-fptrunc,verify" --device-fp64-capabilities=false -S %s \
; RUN:   | %filecheck %s --check-prefixes CHECK,NO-DOUBLE

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK-LABEL: define spir_kernel void @foo(
; DOUBLE: store float %t
; NO-DOUBLE: store float %x
define spir_kernel void @foo(float %x, float* %out) {
  %e = fpext float %x to double
  %t = fptrunc double %e to float
  store float %t, float* %out
  ret void
}

; CHECK-LABEL: define spir_kernel void @foo_optnone(
; DOUBLE: store float %t
; NO-DOUBLE: store float %x
define spir_kernel void @foo_optnone(float %x, float* %out) optnone noinline {
  %e = fpext float %x to double
  %t = fptrunc double %e to float
  store float %t, float* %out
  ret void
}
