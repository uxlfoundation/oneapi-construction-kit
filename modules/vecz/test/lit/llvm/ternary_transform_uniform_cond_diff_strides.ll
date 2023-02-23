; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test_ternary -vecz-passes=ternary-transform -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_ternary(i64 %a, i64 %b, i64* %c) {
entry:
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  %gid_shift = shl i64 %gid, 1
  %cond = icmp eq i64 %a, 0
  %c0 = getelementptr i64, i64* %c, i64 %gid
  store i64 %b, i64* %c0, align 4
  %c1 = getelementptr i64, i64* %c, i64 %gid_shift
  store i64 0, i64* %c1, align 4
  %c2 = select i1 %cond, i64* %c0, i64* %c1
  %c3 = getelementptr i64, i64* %c2, i64 %gid
  store i64 1, i64* %c3, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; This checks that the ternary transform is applied when the condition is
; uniform, and the source GEPs have different constant strides.

; CHECK-GE15: define spir_kernel void @__vecz_v4_test_ternary(i64 %a, i64 %b, ptr %c)
; CHECK-LT15: define spir_kernel void @__vecz_v4_test_ternary(i64 %a, i64 %b, i64* %c)
; CHECK: %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
; CHECK: %gid_shift = shl i64 %gid, 1
; CHECK: %cond = icmp eq i64 %a, 0
; CHECK-GE15: %c0 = getelementptr i64, ptr %c, i64 %gid
; CHECK-LT15: %c0 = getelementptr i64, i64* %c, i64 %gid
; CHECK-GE15: store i64 %b, ptr %c0, align 4
; CHECK-LT15: store i64 %b, i64* %c0, align 4
; CHECK-GE15: %c1 = getelementptr i64, ptr %c, i64 %gid_shift
; CHECK-LT15: %c1 = getelementptr i64, i64* %c, i64 %gid_shift
; CHECK-GE15: store i64 0, ptr %c1, align 4
; CHECK-LT15: store i64 0, i64* %c1, align 4
; CHECK: %[[XOR:.+]] = xor i1 %cond, true
; CHECK-GE15: %[[GEP1:.+]] = getelementptr i64, ptr %c0, i64 %gid
; CHECK-LT15: %[[GEP1:.+]] = getelementptr i64, i64* %c0, i64 %gid
; CHECK-GE15: %[[GEP2:.+]] = getelementptr i64, ptr %c1, i64 %gid
; CHECK-LT15: %[[GEP2:.+]] = getelementptr i64, i64* %c1, i64 %gid
; CHECK-GE15: call void @__vecz_b_masked_store4_mu3ptrb(i64 1, ptr %[[GEP1]], i1 %cond)
; CHECK-LT15: call void @__vecz_b_masked_store4_mPmb(i64 1, i64* %[[GEP1]], i1 %cond)
; CHECK-GE15: call void @__vecz_b_masked_store4_mu3ptrb(i64 1, ptr %[[GEP2]], i1 %[[XOR]])
; CHECK-LT15: call void @__vecz_b_masked_store4_mPmb(i64 1, i64* %[[GEP2]], i1 %[[XOR]])
