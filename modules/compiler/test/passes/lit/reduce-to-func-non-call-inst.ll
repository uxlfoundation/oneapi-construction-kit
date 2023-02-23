; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes "reduce-to-func<names=foo>,verify" -S %s -opaque-pointers | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: define spir_kernel void @foo(
define spir_kernel void @foo() {
  %v = ptrtoint ptr @bar to i32
  ret void
}

; We don't explicitly want to keep @bar but it's used by @foo, which we do.
; CHECK: define spir_kernel void @bar
define spir_kernel void @bar() {
  ret void
}
