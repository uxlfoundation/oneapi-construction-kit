; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test -vecz-passes=builtin-inlining -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(float %a, float %b, i32* %c) {
entry:
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  %cmp = call spir_func i32 @_Z9isgreaterff(float %a, float %b)
  %c0 = getelementptr i32, i32* %c, i64 %gid
  store i32 %cmp, i32* %c0, align 4
  %cmp1 = call spir_func i32 @_Z6islessff(float %a, float %b)
  %c1 = getelementptr i32, i32* %c0, i32 1
  store i32 %cmp1, i32* %c1, align 4
  %cmp2 = call spir_func i32 @_Z7isequalff(float %a, float %b)
  %c2 = getelementptr i32, i32* %c0, i32 2
  store i32 %cmp2, i32* %c2, align 4
  %cmp3 = call spir_func i32 @opt_Z7isequalff(float %a, float %b)
  %c3 = getelementptr i32, i32* %c0, i32 3
  store i32 %cmp3, i32* %c3, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)
declare spir_func i32 @_Z9isgreaterff(float, float)
declare spir_func i32 @_Z6islessff(float, float)
declare spir_func i32 @_Z7isequalff(float, float)

; Test that a non-builtin function is inlined.
define spir_func i32 @opt_Z7isequalff(float, float) {
  ret i32 zeroinitializer
}

; CHECK-GE15: define spir_kernel void @__vecz_v4_test(float %a, float %b, ptr %c)
; CHECK-LT15: define spir_kernel void @__vecz_v4_test(float %a, float %b, i32* %c)
; CHECK: entry:
; CHECK: %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
; CHECK: %relational = fcmp ogt float %a, %b
; CHECK: %relational[[R1:[0-9]+]] = zext i1 %relational to i32
; CHECK-GE15: %c0 = getelementptr i32, ptr %c, i64 %gid
; CHECK-LT15: %c0 = getelementptr i32, i32* %c, i64 %gid
; CHECK-GE15: store i32 %relational[[R1]], ptr %c0, align 4
; CHECK-LT15: store i32 %relational[[R1]], i32* %c0, align 4
; CHECK: %relational[[R2:[0-9]+]] = fcmp olt float %a, %b
; CHECK: %relational[[R3:[0-9]+]] = zext i1 %relational[[R2:[0-9]+]] to i32
; CHECK-GE15: %c1 = getelementptr i32, ptr %c0, {{(i32|i64)}} 1
; CHECK-LT15: %c1 = getelementptr i32, i32* %c0, {{(i32|i64)}} 1
; CHECK-GE15: store i32 %relational[[R3:[0-9]+]], ptr %c1, align 4
; CHECK-LT15: store i32 %relational[[R3:[0-9]+]], i32* %c1, align 4
; CHECK: %relational[[R4:[0-9]+]] = fcmp oeq float %a, %b
; CHECK: %relational[[R5:[0-9]+]] = zext i1 %relational[[R4:[0-9]+]] to i32
; CHECK-GE15: %c2 = getelementptr i32, ptr %c0, {{(i32|i64)}} 2
; CHECK-LT15: %c2 = getelementptr i32, i32* %c0, {{(i32|i64)}} 2
; CHECK-GE15: store i32 %relational[[R5:[0-9]+]], ptr %c2, align 4
; CHECK-LT15: store i32 %relational[[R5:[0-9]+]], i32* %c2, align 4
; CHECK-GE15: %c3 = getelementptr i32, ptr %c0, {{(i32|i64)}} 3
; CHECK-LT15: %c3 = getelementptr i32, i32* %c0, {{(i32|i64)}} 3
; CHECK-GE15: store i32 0, ptr %c3, align 4
; CHECK-LT15: store i32 0, i32* %c3, align 4
; CHECK: ret void
