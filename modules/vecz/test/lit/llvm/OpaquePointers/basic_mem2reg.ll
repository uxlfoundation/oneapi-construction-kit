; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -vecz-passes="function(mem2reg),vecz-mem2reg" -vecz-simd-width=4 -vecz-handle-declaration-only-calls %flag -S < %s | %filecheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(i32 %a, i32 %b, i32* %c, float %rf) {
entry:
  %d = alloca i32
  %e = alloca i32
  %f = alloca float
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  %sum = add i32 %a, %b
  store i32 %sum, i32* %d, align 4
  store i32 %sum, i32* %e, align 4
  %call = call spir_func i32 @foo(i32* %e)
  %d.load = load i32, i32* %d, align 4
  %e.load = load i32, i32* %e, align 4
  %c0 = getelementptr i32, i32* %c, i64 %gid
  store i32 %d.load, i32* %c0, align 4
  %c1 = getelementptr i32, i32* %c0, i64 1
  store i32 %e.load, i32* %c1, align 4
  store float %rf, float* %f
  %ri = bitcast float* %f to i32*
  %ri.load = load i32, i32* %ri, align 4
  %c2 = getelementptr i32, i32* %c1, i64 2
  store i32 %ri.load, i32* %c2, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)
declare spir_func i32 @foo(i32*)

; CHECK: define spir_kernel void @__vecz_v4_test(i32 %a, i32 %b, ptr %c, float %rf)
; CHECK: %e = alloca i32
; CHECK: %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
; CHECK: %sum = add i32 %a, %b
; CHECK: store i32 %sum, ptr %e
; CHECK: %call = call spir_func i32 @foo(ptr{{.*}} %e)
; CHECK: %e.load = load i32, ptr %e
; CHECK: %c0 = getelementptr i32, ptr %c, i64 %gid
; CHECK: store i32 %sum, ptr %c0
; CHECK: %c1 = getelementptr i32, ptr %c0, i64 1
; CHECK: store i32 %e.load, ptr %c1
; CHECK: %0 = bitcast float %rf to i32
; CHECK: %c2 = getelementptr i32, ptr %c1, i64 2
; CHECK: store i32 %0, ptr %c2, align 4
; CHECK: ret void
