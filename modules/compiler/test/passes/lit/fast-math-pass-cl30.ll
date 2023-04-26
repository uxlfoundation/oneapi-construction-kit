; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes fast-math,verify -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

declare float @_Z6lengthf(float)

; CHECK-NOT: @_Z11fast_lengthf

define spir_kernel void @foo() {
  ; CHECK: %l = call float @_Z6lengthf(float 1.000000e+00)
  %l = call float @_Z6lengthf(float 1.0)
  ret void
}

!opencl.ocl.version = !{!0}

!0 = !{i32 3, i32 0}
