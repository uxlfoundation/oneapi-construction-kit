; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test_instructions -vecz-passes=scalarize -vecz-simd-width=4 -vecz-choices=FullScalarization -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @test_instructions(<4 x i32>* %pa, <4 x i32>* %pb, <4 x i32>* %pc) {
entry:
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %a = getelementptr <4 x i32>, <4 x i32>* %pa, i64 %idx
  %b = getelementptr <4 x i32>, <4 x i32>* %pb, i64 %idx
  %c = getelementptr <4 x i32>, <4 x i32>* %pc, i64 %idx
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

; CHECK-GE15: define spir_kernel void @__vecz_v4_test_instructions(ptr %pa, ptr %pb, ptr %pc)
; CHECK-LT15: define spir_kernel void @__vecz_v4_test_instructions(<4 x i32>* %pa, <4 x i32>* %pb, <4 x i32>* %pc)
; CHECK: entry:
; CHECK-GE15: %[[A_0:.+]] = getelementptr i32, ptr %a, i32 0
; CHECK-GE15: %[[A_1:.+]] = getelementptr i32, ptr %a, i32 1
; CHECK-GE15: %[[A_2:.+]] = getelementptr i32, ptr %a, i32 2
; CHECK-GE15: %[[A_3:.+]] = getelementptr i32, ptr %a, i32 3
; CHECK-LT15: %[[A:.+]] = bitcast <4 x i32>* %a to i32*
; CHECK-LT15: %[[A_0:.+]] = getelementptr i32, i32* %[[A]], i32 0
; CHECK-LT15: %[[A_1:.+]] = getelementptr i32, i32* %[[A]], i32 1
; CHECK-LT15: %[[A_2:.+]] = getelementptr i32, i32* %[[A]], i32 2
; CHECK-LT15: %[[A_3:.+]] = getelementptr i32, i32* %[[A]], i32 3
; CHECK-GE15: %[[LA_0:.+]] = load i32, ptr %[[A_0]]
; CHECK-LT15: %[[LA_0:.+]] = load i32, i32* %[[A_0]]
; CHECK-GE15: %[[LA_1:.+]] = load i32, ptr %[[A_1]]
; CHECK-LT15: %[[LA_1:.+]] = load i32, i32* %[[A_1]]
; CHECK-GE15: %[[LA_2:.+]] = load i32, ptr %[[A_2]]
; CHECK-LT15: %[[LA_2:.+]] = load i32, i32* %[[A_2]]
; CHECK-GE15: %[[LA_3:.+]] = load i32, ptr %[[A_3]]
; CHECK-LT15: %[[LA_3:.+]] = load i32, i32* %[[A_3]]
; CHECK-GE15: %[[B_0:.+]] = getelementptr i32, ptr %b, i32 0
; CHECK-GE15: %[[B_1:.+]] = getelementptr i32, ptr %b, i32 1
; CHECK-GE15: %[[B_2:.+]] = getelementptr i32, ptr %b, i32 2
; CHECK-GE15: %[[B_3:.+]] = getelementptr i32, ptr %b, i32 3
; CHECK-LT15: %[[B:.+]] = bitcast <4 x i32>* %b to i32*
; CHECK-LT15: %[[B_0:.+]] = getelementptr i32, i32* %[[B]], i32 0
; CHECK-LT15: %[[B_1:.+]] = getelementptr i32, i32* %[[B]], i32 1
; CHECK-LT15: %[[B_2:.+]] = getelementptr i32, i32* %[[B]], i32 2
; CHECK-LT15: %[[B_3:.+]] = getelementptr i32, i32* %[[B]], i32 3
; CHECK-GE15: %[[LB_0:.+]] = load i32, ptr %[[B_0]]
; CHECK-LT15: %[[LB_0:.+]] = load i32, i32* %[[B_0]]
; CHECK-GE15: %[[LB_1:.+]] = load i32, ptr %[[B_1]]
; CHECK-LT15: %[[LB_1:.+]] = load i32, i32* %[[B_1]]
; CHECK-GE15: %[[LB_2:.+]] = load i32, ptr %[[B_2]]
; CHECK-LT15: %[[LB_2:.+]] = load i32, i32* %[[B_2]]
; CHECK-GE15: %[[LB_3:.+]] = load i32, ptr %[[B_3]]
; CHECK-LT15: %[[LB_3:.+]] = load i32, i32* %[[B_3]]
; CHECK: %[[ADD1:.+]] = add i32 %[[LB_0]], %[[LA_0]]
; CHECK: %[[ADD2:.+]] = add i32 %[[LB_1]], %[[LA_1]]
; CHECK: %[[ADD3:.+]] = add i32 %[[LB_2]], %[[LA_2]]
; CHECK: %[[ADD4:.+]] = add i32 %[[LB_3]], %[[LA_3]]
; CHECK-GE15: %[[C_0:.+]] = getelementptr i32, ptr %c, i32 0
; CHECK-GE15: %[[C_1:.+]] = getelementptr i32, ptr %c, i32 1
; CHECK-GE15: %[[C_2:.+]] = getelementptr i32, ptr %c, i32 2
; CHECK-GE15: %[[C_3:.+]] = getelementptr i32, ptr %c, i32 3
; CHECK-LT15: %[[C:.+]] = bitcast <4 x i32>* %c to i32*
; CHECK-LT15: %[[C_0:.+]] = getelementptr i32, i32* %[[C]], i32 0
; CHECK-LT15: %[[C_1:.+]] = getelementptr i32, i32* %[[C]], i32 1
; CHECK-LT15: %[[C_2:.+]] = getelementptr i32, i32* %[[C]], i32 2
; CHECK-LT15: %[[C_3:.+]] = getelementptr i32, i32* %[[C]], i32 3
; CHECK-GE15: store i32 %[[ADD1]], ptr %[[C_0]]
; CHECK-LT15: store i32 %[[ADD1]], i32* %[[C_0]]
; CHECK-GE15: store i32 %[[ADD2]], ptr %[[C_1]]
; CHECK-LT15: store i32 %[[ADD2]], i32* %[[C_1]]
; CHECK-GE15: store i32 %[[ADD3]], ptr %[[C_2]]
; CHECK-LT15: store i32 %[[ADD3]], i32* %[[C_2]]
; CHECK-GE15: store i32 %[[ADD4]], ptr %[[C_3]]
; CHECK-LT15: store i32 %[[ADD4]], i32* %[[C_3]]
; CHECK-GE15: %arrayidx3 = getelementptr inbounds <4 x i32>, ptr %a, i64 1
; CHECK-LT15: %arrayidx3 = getelementptr inbounds <4 x i32>, <4 x i32>* %a, i64 1
; CHECK-GE15: %[[A1_0:.+]] = getelementptr inbounds i32, ptr %arrayidx3, i32 0
; CHECK-GE15: %[[A1_1:.+]] = getelementptr inbounds i32, ptr %arrayidx3, i32 1
; CHECK-GE15: %[[A1_2:.+]] = getelementptr inbounds i32, ptr %arrayidx3, i32 2
; CHECK-GE15: %[[A1_3:.+]] = getelementptr inbounds i32, ptr %arrayidx3, i32 3
; CHECK-LT15: %[[A1:.+]] = bitcast <4 x i32>* %arrayidx3 to i32*
; CHECK-LT15: %[[A1_0:.+]] = getelementptr inbounds i32, i32* %[[A1]], i32 0
; CHECK-LT15: %[[A1_1:.+]] = getelementptr inbounds i32, i32* %[[A1]], i32 1
; CHECK-LT15: %[[A1_2:.+]] = getelementptr inbounds i32, i32* %[[A1]], i32 2
; CHECK-LT15: %[[A1_3:.+]] = getelementptr inbounds i32, i32* %[[A1]], i32 3
; CHECK-GE15: %[[LA1_0:.+]] = load i32, ptr %[[A1_0]]
; CHECK-LT15: %[[LA1_0:.+]] = load i32, i32* %[[A1_0]]
; CHECK-GE15: %[[LA1_1:.+]] = load i32, ptr %[[A1_1]]
; CHECK-LT15: %[[LA1_1:.+]] = load i32, i32* %[[A1_1]]
; CHECK-GE15: %[[LA1_2:.+]] = load i32, ptr %[[A1_2]]
; CHECK-LT15: %[[LA1_2:.+]] = load i32, i32* %[[A1_2]]
; CHECK-GE15: %[[LA1_3:.+]] = load i32, ptr %[[A1_3]]
; CHECK-LT15: %[[LA1_3:.+]] = load i32, i32* %[[A1_3]]
; CHECK-GE15: %arrayidx4 = getelementptr inbounds <4 x i32>, ptr %b, i64 1
; CHECK-LT15: %arrayidx4 = getelementptr inbounds <4 x i32>, <4 x i32>* %b, i64 1
; CHECK-GE15: %[[B1_0:.+]] = getelementptr inbounds i32, ptr %arrayidx4, i32 0
; CHECK-GE15: %[[B1_1:.+]] = getelementptr inbounds i32, ptr %arrayidx4, i32 1
; CHECK-GE15: %[[B1_2:.+]] = getelementptr inbounds i32, ptr %arrayidx4, i32 2
; CHECK-GE15: %[[B1_3:.+]] = getelementptr inbounds i32, ptr %arrayidx4, i32 3
; CHECK-LT15: %[[B1:.+]] = bitcast <4 x i32>* %arrayidx4 to i32*
; CHECK-LT15: %[[B1_0:.+]] = getelementptr inbounds i32, i32* %[[B1]], i32 0
; CHECK-LT15: %[[B1_1:.+]] = getelementptr inbounds i32, i32* %[[B1]], i32 1
; CHECK-LT15: %[[B1_2:.+]] = getelementptr inbounds i32, i32* %[[B1]], i32 2
; CHECK-LT15: %[[B1_3:.+]] = getelementptr inbounds i32, i32* %[[B1]], i32 3
; CHECK-GE15: %[[LB1_0:.+]] = load i32, ptr %[[B1_0]]
; CHECK-LT15: %[[LB1_0:.+]] = load i32, i32* %[[B1_0]]
; CHECK-GE15: %[[LB1_1:.+]] = load i32, ptr %[[B1_1]]
; CHECK-LT15: %[[LB1_1:.+]] = load i32, i32* %[[B1_1]]
; CHECK-GE15: %[[LB1_2:.+]] = load i32, ptr %[[B1_2]]
; CHECK-LT15: %[[LB1_2:.+]] = load i32, i32* %[[B1_2]]
; CHECK-GE15: %[[LB1_3:.+]] = load i32, ptr %[[B1_3]]
; CHECK-LT15: %[[LB1_3:.+]] = load i32, i32* %[[B1_3]]
; CHECK: %[[CMP5:.+]] = icmp sgt i32 %[[LA1_0]], %[[LB1_0]]
; CHECK: %[[CMP6:.+]] = icmp sgt i32 %[[LA1_1]], %[[LB1_1]]
; CHECK: %[[CMP8:.+]] = icmp sgt i32 %[[LA1_2]], %[[LB1_2]]
; CHECK: %[[CMP9:.+]] = icmp sgt i32 %[[LA1_3]], %[[LB1_3]]
; CHECK: %[[SEXT10:.+]] = sext i1 %[[CMP5]] to i32
; CHECK: %[[SEXT11:.+]] = sext i1 %[[CMP6]] to i32
; CHECK: %[[SEXT12:.+]] = sext i1 %[[CMP8]] to i32
; CHECK: %[[SEXT13:.+]] = sext i1 %[[CMP9]] to i32
; CHECK-GE15: %arrayidx5 = getelementptr inbounds <4 x i32>, ptr %c, i64 1
; CHECK-LT15: %arrayidx5 = getelementptr inbounds <4 x i32>, <4 x i32>* %c, i64 1
; CHECK-GE15: %[[C1_0:.+]] = getelementptr inbounds i32, ptr %arrayidx5, i32 0
; CHECK-GE15: %[[C1_1:.+]] = getelementptr inbounds i32, ptr %arrayidx5, i32 1
; CHECK-GE15: %[[C1_2:.+]] = getelementptr inbounds i32, ptr %arrayidx5, i32 2
; CHECK-GE15: %[[C1_3:.+]] = getelementptr inbounds i32, ptr %arrayidx5, i32 3
; CHECK-LT15: %[[C1:.+]] = bitcast <4 x i32>* %arrayidx5 to i32*
; CHECK-LT15: %[[C1_0:.+]] = getelementptr inbounds i32, i32* %[[C1]], i32 0
; CHECK-LT15: %[[C1_1:.+]] = getelementptr inbounds i32, i32* %[[C1]], i32 1
; CHECK-LT15: %[[C1_2:.+]] = getelementptr inbounds i32, i32* %[[C1]], i32 2
; CHECK-LT15: %[[C1_3:.+]] = getelementptr inbounds i32, i32* %[[C1]], i32 3
; CHECK-GE15: store i32 %[[SEXT10]], ptr %[[C1_0]]
; CHECK-LT15: store i32 %[[SEXT10]], i32* %[[C1_0]]
; CHECK-GE15: store i32 %[[SEXT11]], ptr %[[C1_1]]
; CHECK-LT15: store i32 %[[SEXT11]], i32* %[[C1_1]]
; CHECK-GE15: store i32 %[[SEXT12]], ptr %[[C1_2]]
; CHECK-LT15: store i32 %[[SEXT12]], i32* %[[C1_2]]
; CHECK-GE15: store i32 %[[SEXT13]], ptr %[[C1_3]]
; CHECK-LT15: store i32 %[[SEXT13]], i32* %[[C1_3]]
; CHECK-GE15: %arrayidx6 = getelementptr inbounds <4 x i32>, ptr %a, i64 2
; CHECK-LT15: %arrayidx6 = getelementptr inbounds <4 x i32>, <4 x i32>* %a, i64 2
; CHECK-GE15: %[[A2_0:.+]] = getelementptr inbounds i32, ptr %arrayidx6, i32 0
; CHECK-GE15: %[[A2_1:.+]] = getelementptr inbounds i32, ptr %arrayidx6, i32 1
; CHECK-GE15: %[[A2_2:.+]] = getelementptr inbounds i32, ptr %arrayidx6, i32 2
; CHECK-GE15: %[[A2_3:.+]] = getelementptr inbounds i32, ptr %arrayidx6, i32 3
; CHECK-LT15: %[[A2:.+]] = bitcast <4 x i32>* %arrayidx6 to i32*
; CHECK-LT15: %[[A2_0:.+]] = getelementptr inbounds i32, i32* %[[A2]], i32 0
; CHECK-LT15: %[[A2_1:.+]] = getelementptr inbounds i32, i32* %[[A2]], i32 1
; CHECK-LT15: %[[A2_2:.+]] = getelementptr inbounds i32, i32* %[[A2]], i32 2
; CHECK-LT15: %[[A2_3:.+]] = getelementptr inbounds i32, i32* %[[A2]], i32 3
; CHECK-GE15: %[[LA2_0:.+]] = load i32, ptr %[[A2_0]]
; CHECK-LT15: %[[LA2_0:.+]] = load i32, i32* %[[A2_0]]
; CHECK-GE15: %[[LA2_1:.+]] = load i32, ptr %[[A2_1]]
; CHECK-LT15: %[[LA2_1:.+]] = load i32, i32* %[[A2_1]]
; CHECK-GE15: %[[LA2_2:.+]] = load i32, ptr %[[A2_2]]
; CHECK-LT15: %[[LA2_2:.+]] = load i32, i32* %[[A2_2]]
; CHECK-GE15: %[[LA2_3:.+]] = load i32, ptr %[[A2_3]]
; CHECK-LT15: %[[LA2_3:.+]] = load i32, i32* %[[A2_3]]
; CHECK: %[[CMP714:.+]] = icmp slt i32 %[[LA2_0]], 11
; CHECK: %[[CMP715:.+]] = icmp slt i32 %[[LA2_1]], 12
; CHECK: %[[CMP716:.+]] = icmp slt i32 %[[LA2_2]], 13
; CHECK: %[[CMP717:.+]] = icmp slt i32 %[[LA2_3]], 14
; CHECK: %[[SEXT818:.+]] = sext i1 %[[CMP714]] to i32
; CHECK: %[[SEXT819:.+]] = sext i1 %[[CMP715]] to i32
; CHECK: %[[SEXT820:.+]] = sext i1 %[[CMP716]] to i32
; CHECK: %[[SEXT821:.+]] = sext i1 %[[CMP717]] to i32
; CHECK-GE15: %arrayidx9 = getelementptr inbounds <4 x i32>, ptr %c, i64 2
; CHECK-LT15: %arrayidx9 = getelementptr inbounds <4 x i32>, <4 x i32>* %c, i64 2
; CHECK-GE15: %[[C2_0:.+]] = getelementptr inbounds i32, ptr %arrayidx9, i32 0
; CHECK-GE15: %[[C2_1:.+]] = getelementptr inbounds i32, ptr %arrayidx9, i32 1
; CHECK-GE15: %[[C2_2:.+]] = getelementptr inbounds i32, ptr %arrayidx9, i32 2
; CHECK-GE15: %[[C2_3:.+]] = getelementptr inbounds i32, ptr %arrayidx9, i32 3
; CHECK-LT15: %[[C2:.+]] = bitcast <4 x i32>* %arrayidx9 to i32*
; CHECK-LT15: %[[C2_0:.+]] = getelementptr inbounds i32, i32* %[[C2]], i32 0
; CHECK-LT15: %[[C2_1:.+]] = getelementptr inbounds i32, i32* %[[C2]], i32 1
; CHECK-LT15: %[[C2_2:.+]] = getelementptr inbounds i32, i32* %[[C2]], i32 2
; CHECK-LT15: %[[C2_3:.+]] = getelementptr inbounds i32, i32* %[[C2]], i32 3
; CHECK-GE15: store i32 %[[SEXT818]], ptr %[[C2_0]]
; CHECK-LT15: store i32 %[[SEXT818]], i32* %[[C2_0]]
; CHECK-GE15: store i32 %[[SEXT819]], ptr %[[C2_1]]
; CHECK-LT15: store i32 %[[SEXT819]], i32* %[[C2_1]]
; CHECK-GE15: store i32 %[[SEXT820]], ptr %[[C2_2]]
; CHECK-LT15: store i32 %[[SEXT820]], i32* %[[C2_2]]
; CHECK-GE15: store i32 %[[SEXT821]], ptr %[[C2_3]]
; CHECK-LT15: store i32 %[[SEXT821]], i32* %[[C2_3]]
; CHECK: ret void
