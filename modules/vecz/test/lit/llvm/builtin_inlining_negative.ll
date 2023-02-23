; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test_rhadd -vecz-passes=builtin-inlining -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_normalize(float %a, float %b, i32* %c) {
entry:
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  %norm = call spir_func float @_Z9normalizef(float %a)
  %normi = fptosi float %norm to i32
  %c0 = getelementptr i32, i32* %c, i64 %gid
  store i32 %normi, i32* %c0, align 4
  ret void
}

define spir_kernel void @test_rhadd(i32 %a, i32 %b, i32* %c) {
entry:
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  %add = call spir_func i32 @_Z5rhaddjj(i32 %a, i32 %b)
  %c0 = getelementptr i32, i32* %c, i64 %gid
  store i32 %add, i32* %c0, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)
declare spir_func float @_Z9normalizef(float)
declare spir_func i32 @_Z5rhaddjj(i32, i32)

; CHECK-NOT-GE15: define spir_kernel void @__vecz_v4_test_normalize(float %a, float %b, ptr %c)
; CHECK-NOT-LT15: define spir_kernel void @__vecz_v4_test_normalize(float %a, float %b, i32* %c)

; CHECK-GE15: define spir_kernel void @__vecz_v4_test_rhadd(i32 %a, i32 %b, ptr %c)
; CHECK-LT15: define spir_kernel void @__vecz_v4_test_rhadd(i32 %a, i32 %b, i32* %c)
; CHECK: entry:
; CHECK: %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
; CHECK: %add = call spir_func i32 @_Z5rhaddjj(i32 %a, i32 %b)
; CHECK-GE15: %c0 = getelementptr i32, ptr %c, i64 %gid
; CHECK-LT15: %c0 = getelementptr i32, i32* %c, i64 %gid
; CHECK-GE15: store i32 %add, ptr %c0, align 4
; CHECK-LT15: store i32 %add, i32* %c0, align 4
; CHECK: ret void
