; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -k test -S < %s | %filecheck %s

; ModuleID = 'kernel.opencl'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(void (i32)*, i32) {
entry:
  call void %0 (i32 %1)
  ret void
}

; This is really a check to see if opt crashed or not
; CHECK: define spir_kernel void @test(
