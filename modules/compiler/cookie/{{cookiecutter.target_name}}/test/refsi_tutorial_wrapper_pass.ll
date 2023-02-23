; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --device "RefSi M1 Tutorial" %s --passes refsi-wrapper,verify -S | %filecheck %s

target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
target triple = "riscv64-unknown-unknown-elf"

; CHECK: define void @add.refsi-wrapper(i64 %instance, i64 %slice, ptr nocapture readonly %args, ptr nocapture readonly %wg)
; CHECK: [[WG_INFO:%.*]] = alloca %MuxWorkGroupInfo
; CHECK: [[GEP_NUM_GROUPS_Y:%.*]] = getelementptr %MuxWorkGroupInfo, ptr %wg, i32 0, i32 1, i32 1
; CHECK: [[NUM_GROUPS_Y:%.*]] = load i64, ptr [[GEP_NUM_GROUPS_Y]]
; CHECK: [[SECOND_DIM:%.*]] = urem i64 %slice, [[NUM_GROUPS_Y]]
; CHECK: [[THIRD_DIM:%.*]] = udiv i64 %slice, [[NUM_GROUPS_Y]]
; CHECK: [[GEP_GROUP_ID_0:%.*]] = getelementptr %MuxWorkGroupInfo, ptr [[WG_INFO]], i32 0, i32 0, i32 0
; CHECK: store i64 %instance, ptr [[GEP_GROUP_ID_0]]
; CHECK: [[GEP_GROUP_ID_1:%.*]] = getelementptr %MuxWorkGroupInfo, ptr [[WG_INFO]], i32 0, i32 0, i32 1
; CHECK: store i64 [[SECOND_DIM]], ptr [[GEP_GROUP_ID_1]]
; CHECK: [[GEP_GROUP_ID_2:%.*]] = getelementptr %MuxWorkGroupInfo, ptr [[WG_INFO]], i32 0, i32 0, i32 2
; CHECK: store i64 [[THIRD_DIM]], ptr [[GEP_GROUP_ID_2]]
; CHECK: call void @add(ptr nocapture readonly [[ARGS:%.*]], ptr nocapture readonly [[WG_INFO]])

%MuxPackedArgs.add = type { i32 addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)* }
%MuxWorkGroupInfo = type { [3 x i64], [3 x i64], [3 x i64], [3 x i64], i32 }

; Function Attrs: nounwind
define void @add(%MuxPackedArgs.add* nocapture readonly %args, %MuxWorkGroupInfo* nocapture readonly %wg) #0 {
  ret void
}

attributes #0 = { "mux-kernel"="entry-point" }
