; Copyright (C) Codeplay Software Limited
;
; Licensed under the Apache License, Version 2.0 (the "License") with LLVM
; Exceptions; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

; RUN: muxc --passes work-item-loops,verify -S %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

; CHECK: %minimal_barrier_2_live_mem_info = type { i{{[^,]+}}, i{{[^,]+}}, i{{[^,]+}}{{(, \[[0-9]+ x i8\])?}} }

@minimal_barrier_2.cache = internal unnamed_addr addrspace(3) global [4 x i32] undef, align 4

define void @minimal_barrier_2(i32 addrspace(1)* %output) #0 {
entry:
  %call1 = tail call i64 @__mux_get_local_id(i32 0)
  %conv2 = trunc i64 %call1 to i32
  %call457 = tail call i64 @__mux_get_local_size(i32 0)
  %cmp58.not = icmp eq i64 %call457, 0
  br i1 %cmp58.not, label %for.cond.cleanup, label %for.body.lr.ph

for.body.lr.ph:                                   ; preds = %entry
  %sext = shl i64 %call1, 32
  %idxprom = ashr exact i64 %sext, 32
  %arrayidx = getelementptr inbounds [4 x i32], [4 x i32] addrspace(3)* @minimal_barrier_2.cache, i64 0, i64 %idxprom
  br label %for.body

for.cond.cleanup:                                 ; preds = %for.inc30, %entry
  %my_value.0.lcssa = phi i32 [ 0, %entry ], [ %my_value.3, %for.inc30 ]
  %call36 = tail call i64 @__mux_get_global_id(i32 0)
  %cmp37 = icmp eq i64 %call36, 1
  br i1 %cmp37, label %if.then39, label %if.end41

for.body:                                         ; preds = %for.inc30, %for.body.lr.ph
  %call462 = phi i64 [ %call457, %for.body.lr.ph ], [ %call4, %for.inc30 ]
  %conv361 = phi i64 [ 0, %for.body.lr.ph ], [ %conv3, %for.inc30 ]
  %i.060 = phi i32 [ 0, %for.body.lr.ph ], [ %conv34, %for.inc30 ]
  %my_value.059 = phi i32 [ 0, %for.body.lr.ph ], [ %my_value.3, %for.inc30 ]
  %div51 = lshr i64 %call462, 1
  %cmp8 = icmp ult i64 %div51, %conv361
  br i1 %cmp8, label %if.then, label %if.end

if.then:                                          ; preds = %for.body
  %0 = load i32, i32 addrspace(3)* %arrayidx, align 4
  br label %if.end

if.end:                                           ; preds = %if.then, %for.body
  %my_value.1 = phi i32 [ %0, %if.then ], [ %my_value.059, %for.body ]
  %call1252 = tail call i64 @__mux_get_local_size(i32 0)
  %cmp1353.not = icmp eq i64 %call1252, 0
  br i1 %cmp1353.not, label %for.cond.cleanup15, label %for.body16

for.cond.cleanup15:                               ; preds = %for.body16, %if.end
  %my_value.2.lcssa = phi i32 [ %my_value.1, %if.end ], [ %add21, %for.body16 ]
  %call12.lcssa = phi i64 [ 0, %if.end ], [ %call12, %for.body16 ]
  %call22 = tail call i64 @__mux_get_local_id(i32 0)
  %cmp24 = icmp ugt i64 %call22, %call12.lcssa
  br i1 %cmp24, label %if.then26, label %for.inc30

for.body16:                                       ; preds = %for.body16, %if.end
  %j.055 = phi i32 [ %inc, %for.body16 ], [ 0, %if.end ]
  %my_value.254 = phi i32 [ %add21, %for.body16 ], [ %my_value.1, %if.end ]
  store i32 %conv2, i32 addrspace(3)* %arrayidx, align 4
  tail call void @__mux_work_group_barrier(i32 0, i32 1, i32 272)
  %1 = load i32, i32 addrspace(3)* getelementptr inbounds ([4 x i32], [4 x i32] addrspace(3)* @minimal_barrier_2.cache, i64 0, i64 0), align 4
  %add = add nsw i32 %1, %my_value.254
  %2 = load i32, i32 addrspace(3)* getelementptr inbounds ([4 x i32], [4 x i32] addrspace(3)* @minimal_barrier_2.cache, i64 0, i64 1), align 4
  %add19 = add nsw i32 %add, %2
  %3 = load i32, i32 addrspace(3)* getelementptr inbounds ([4 x i32], [4 x i32] addrspace(3)* @minimal_barrier_2.cache, i64 0, i64 2), align 4
  %add20 = add nsw i32 %add19, %3
  %4 = load i32, i32 addrspace(3)* getelementptr inbounds ([4 x i32], [4 x i32] addrspace(3)* @minimal_barrier_2.cache, i64 0, i64 3), align 4
  %add21 = add nsw i32 %add20, %4
  %inc = add i32 %j.055, 1
  %conv11 = zext i32 %inc to i64
  %call12 = tail call i64 @__mux_get_local_size(i32 0)
  %cmp13 = icmp ugt i64 %call12, %conv11
  br i1 %cmp13, label %for.body16, label %for.cond.cleanup15

if.then26:                                        ; preds = %for.cond.cleanup15
  %arrayidx28 = getelementptr inbounds [4 x i32], [4 x i32] addrspace(3)* @minimal_barrier_2.cache, i64 0, i64 %call22
  %5 = load i32, i32 addrspace(3)* %arrayidx28, align 4
  br label %for.inc30

for.inc30:                                        ; preds = %if.then26, %for.cond.cleanup15
  %my_value.3 = phi i32 [ %5, %if.then26 ], [ %my_value.2.lcssa, %for.cond.cleanup15 ]
  %6 = trunc i64 %call12.lcssa to i32
  %conv34 = add i32 %i.060, %6
  %conv3 = zext i32 %conv34 to i64
  %call4 = tail call i64 @__mux_get_local_size(i32 0)
  %cmp = icmp ugt i64 %call4, %conv3
  br i1 %cmp, label %for.body, label %for.cond.cleanup

if.then39:                                        ; preds = %for.cond.cleanup
  store i32 %my_value.0.lcssa, i32 addrspace(1)* %output, align 4
  br label %if.end41

if.end41:                                         ; preds = %if.then39, %for.cond.cleanup
  ret void
}

declare void @__mux_work_group_barrier(i32, i32, i32)

declare i64 @__mux_get_global_id(i32)

declare i64 @__mux_get_local_size(i32)

declare i64 @__mux_get_local_id(i32)

declare i64 @__mux_get_group_id(i32)

declare i64 @__mux_get_global_offset(i32)

declare void @__mux_set_local_id(i32, i64)

attributes #0 = { "mux-kernel"="entry-point" }
