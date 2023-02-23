; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes optimal-builtin-replace,verify -S %s | %filecheck %s

target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
target triple = "spirv64-unknown-unknown"

; CHECK: call float @llvm.minnum.f32(float %a, float %b)
define spir_kernel void @do_fmin(float %a, float %b) {
  %v = call float @_Z13__abacus_fmin(float %a, float %b)
  ret void
}

declare float @_Z13__abacus_fmin(float, float)
