; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test_negative -vecz-passes=ternary-transform -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_negative(i64 %a, i64 %b, i64* %c) {
entry:
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  %cond = icmp eq i64 %a, %gid
  %c0 = getelementptr i64, i64* %c, i64 %gid
  %c1 = getelementptr i64, i64* %c, i64 0
  %c2 = select i1 %cond, i64* %c0, i64* %c1
  store i64 %b, i64* %c2, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; This checks that the ternary transform is not applied when the select is not
; accessed through an additional GEP.

; CHECK-GE15: define spir_kernel void @__vecz_v4_test_negative(i64 %a, i64 %b, ptr %c)
; CHECK-LT15: define spir_kernel void @__vecz_v4_test_negative(i64 %a, i64 %b, i64* %c)
; CHECK: %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
; CHECK: %cond = icmp eq i64 %a, %gid
; CHECK-GE15: %c0 = getelementptr i64, ptr %c, i64 %gid
; CHECK-LT15: %c0 = getelementptr i64, i64* %c, i64 %gid
; CHECK-GE15: %c1 = getelementptr i64, ptr %c, i64 0
; CHECK-LT15: %c1 = getelementptr i64, i64* %c, i64 0
; CHECK-GE15: %c2 = select i1 %cond, ptr %c0, ptr %c1
; CHECK-LT15: %c2 = select i1 %cond, i64* %c0, i64* %c1
; CHECK-GE15: store i64 %b, ptr %c2, align 4
; CHECK-LT15: store i64 %b, i64* %c2, align 4
