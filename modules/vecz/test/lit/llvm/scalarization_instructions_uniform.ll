; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test_instructions -vecz-passes=scalarize -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_instructions(<4 x i32>* %a, <4 x i32>* %b, <4 x i32>* %c) {
entry:
  %0 = load <4 x i32>, <4 x i32>* %a, align 16
  %1 = load <4 x i32>, <4 x i32>* %b, align 16
  %add = add <4 x i32> %1, %0
  store <4 x i32> %add, <4 x i32>* %c, align 16
  %arrayidx3 = getelementptr inbounds <4 x i32>, <4 x i32>* %a, i64 1
  %2 = load <4 x i32>, <4 x i32>* %arrayidx3, align 16
  %arrayidx4 = getelementptr inbounds <4 x i32>, <4 x i32>* %b, i64 1
  %3 = load <4 x i32>, <4 x i32>* %arrayidx4, align 16
  %cmp = icmp sgt <4 x i32> %2, %3
  %sext = sext <4 x i1> %cmp to <4 x i32>
  %arrayidx5 = getelementptr inbounds <4 x i32>, <4 x i32>* %c, i64 1
  store <4 x i32> %sext, <4 x i32>* %arrayidx5, align 16
  %arrayidx6 = getelementptr inbounds <4 x i32>, <4 x i32>* %a, i64 2
  %4 = load <4 x i32>, <4 x i32>* %arrayidx6, align 16
  %cmp7 = icmp slt <4 x i32> %4, <i32 11, i32 12, i32 13, i32 14>
  %sext8 = sext <4 x i1> %cmp7 to <4 x i32>
  %arrayidx9 = getelementptr inbounds <4 x i32>, <4 x i32>* %c, i64 2
  store <4 x i32> %sext8, <4 x i32>* %arrayidx9, align 16
  ret void
}

; Checks that this function gets vectorized, although because every instruction is
; uniform, the process of vectorization makes no actual changes whatsoever!
; CHECK-GE15: define spir_kernel void @__vecz_v4_test_instructions(ptr %a, ptr %b, ptr %c)
; CHECK-LT15: define spir_kernel void @__vecz_v4_test_instructions(<4 x i32>* %a, <4 x i32>* %b, <4 x i32>* %c)
; CHECK: entry:
; CHECK-GE15: %[[LA:.+]] = load <4 x i32>, ptr %a, align 16
; CHECK-LT15: %[[LA:.+]] = load <4 x i32>, <4 x i32>* %a, align 16
; CHECK-GE15: %[[LB:.+]] = load <4 x i32>, ptr %b, align 16
; CHECK-LT15: %[[LB:.+]] = load <4 x i32>, <4 x i32>* %b, align 16
; CHECK: %[[ADD:.+]] = add <4 x i32> %[[LB]], %[[LA]]
; CHECK-GE15: store <4 x i32> %[[ADD]], ptr %c, align 16
; CHECK-LT15: store <4 x i32> %[[ADD]], <4 x i32>* %c, align 16
; CHECK-GE15: %[[A1:.+]] = getelementptr inbounds <4 x i32>, ptr %a, i64 1
; CHECK-LT15: %[[A1:.+]] = getelementptr inbounds <4 x i32>, <4 x i32>* %a, i64 1
; CHECK-GE15: %[[LA1:.+]] = load <4 x i32>, ptr %[[A1]], align 16
; CHECK-LT15: %[[LA1:.+]] = load <4 x i32>, <4 x i32>* %[[A1]], align 16
; CHECK-GE15: %[[B1:.+]] = getelementptr inbounds <4 x i32>, ptr %b, i64 1
; CHECK-LT15: %[[B1:.+]] = getelementptr inbounds <4 x i32>, <4 x i32>* %b, i64 1
; CHECK-GE15: %[[LB1:.+]] = load <4 x i32>, ptr %[[B1]], align 16
; CHECK-LT15: %[[LB1:.+]] = load <4 x i32>, <4 x i32>* %[[B1]], align 16
; CHECK: %[[CMP:.+]] = icmp sgt <4 x i32> %[[LA1]], %[[LB1]]
; CHECK: %[[SEXT:.+]] = sext <4 x i1> %[[CMP]] to <4 x i32>
; CHECK-GE15: %[[C1:.+]] = getelementptr inbounds <4 x i32>, ptr %c, i64 1
; CHECK-LT15: %[[C1:.+]] = getelementptr inbounds <4 x i32>, <4 x i32>* %c, i64 1
; CHECK-GE15: store <4 x i32> %[[SEXT]], ptr %[[C1]], align 16
; CHECK-LT15: store <4 x i32> %[[SEXT]], <4 x i32>* %[[C1]], align 16
; CHECK-GE15: %[[A2:.+]] = getelementptr inbounds <4 x i32>, ptr %a, i64 2
; CHECK-LT15: %[[A2:.+]] = getelementptr inbounds <4 x i32>, <4 x i32>* %a, i64 2
; CHECK-GE15: %[[LA2:.+]] = load <4 x i32>, ptr %[[A2]], align 16
; CHECK-LT15: %[[LA2:.+]] = load <4 x i32>, <4 x i32>* %[[A2]], align 16
; CHECK: %[[CMP7:.+]] = icmp slt <4 x i32> %[[LA2]], <i32 11, i32 12, i32 13, i32 14>
; CHECK: %[[SEXT8:.+]] = sext <4 x i1> %[[CMP7]] to <4 x i32>
; CHECK-GE15: %[[C2:.+]] = getelementptr inbounds <4 x i32>, ptr %c, i64 2
; CHECK-LT15: %[[C2:.+]] = getelementptr inbounds <4 x i32>, <4 x i32>* %c, i64 2
; CHECK-GE15: store <4 x i32> %[[SEXT8]], ptr %[[C2]], align 16
; CHECK-LT15: store <4 x i32> %[[SEXT8]], <4 x i32>* %[[C2]], align 16
; CHECK: ret void
