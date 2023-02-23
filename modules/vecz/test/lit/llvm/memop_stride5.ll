; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -S < %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @test(i32 addrspace(1)* %src, i32 addrspace(1)* %dst, i32 %n) {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %0 = load i32, i32 addrspace(1)* %src, align 4
  %mul = mul nuw nsw i64 %call, 5
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %dst, i64 %mul
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; CHECK: spir_kernel void @test
; CHECK: _interleaved_
