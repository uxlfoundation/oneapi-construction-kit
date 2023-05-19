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

; RUN: %muxc --passes barriers-pass,verify -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

; CHECK: %minimal_barrier_live_mem_info = type { {{[^,]+}}, {{[^,]+}}, {{[^,]+}}{{(, \[[0-9]+ x i8\])?}} }

define void @minimal_barrier(i32 %min_0, i32 %min_1, i32 %stride, i32 %n0, i32 %n1, i32 %n2, i32 addrspace(1)* %g, i32 addrspace(3)* align 64 %shared) #0 {
entry:
  %call = tail call i64 @_Z12get_group_idj(i32 1)
  %conv = trunc i64 %call to i32
  %call1 = tail call i64 @_Z12get_group_idj(i32 0)
  %conv2 = trunc i64 %call1 to i32
  %call3 = tail call i64 @_Z12get_local_idj(i32 1)
  %conv4 = trunc i64 %call3 to i32
  %call5 = tail call i64 @_Z12get_local_idj(i32 0)
  %conv6 = trunc i64 %call5 to i32
  %mul = shl nsw i32 %conv, 3
  %add = add nsw i32 %mul, %min_1
  %call7 = tail call i32 @_Z3minii(i32 %add, i32 %n0)
  %mul8 = shl nsw i32 %conv2, 4
  %add9 = add nsw i32 %mul8, %min_0
  %call10 = tail call i32 @_Z3minii(i32 %add9, i32 %n1)
  tail call void @__mux_work_group_barrier(i32 0, i32 2, i32 272)
  %cmp = icmp slt i32 %conv4, 1
  %cmp12 = icmp slt i32 %conv6, 16
  %or.cond = select i1 %cmp, i1 %cmp12, i1 false
  br i1 %or.cond, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %mul14 = mul nsw i32 %conv6, 20
  %add15 = add nsw i32 %mul14, 9
  %add16 = add nsw i32 %mul14, 10
  %0 = sext i32 %add16 to i64
  br label %for.body

for.body:                                         ; preds = %for.body, %if.then
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.body ], [ 0, %if.then ]
  %1 = trunc i64 %indvars.iv to i32
  %sub = sub i32 %add15, %1
  %idxprom = sext i32 %sub to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(3)* %shared, i64 %idxprom
  %2 = load i32, i32 addrspace(3)* %arrayidx, align 4
  %3 = add nsw i64 %indvars.iv, %0
  %arrayidx21 = getelementptr inbounds i32, i32 addrspace(3)* %shared, i64 %3
  store i32 %2, i32 addrspace(3)* %arrayidx21, align 4
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond.not = icmp eq i64 %indvars.iv.next, 10
  br i1 %exitcond.not, label %if.end, label %for.body

if.end:                                           ; preds = %for.body, %entry
  tail call void @__mux_work_group_barrier(i32 1, i32 2, i32 272)
  br i1 %cmp12, label %if.then24, label %if.end35

if.then24:                                        ; preds = %if.end
  %add26 = add nsw i32 %call7, %conv4
  %mul27 = mul nsw i32 %add26, %stride
  %sub25 = sub i32 %conv6, %n2
  %add28 = add i32 %sub25, %call10
  %add29 = add i32 %add28, %mul27
  %4 = load i32, i32 addrspace(3)* %shared, align 64
  %arrayidx31 = getelementptr inbounds i32, i32 addrspace(3)* %shared, i64 149
  %5 = load i32, i32 addrspace(3)* %arrayidx31, align 4
  %add32 = add nsw i32 %5, %4
  %idxprom33 = sext i32 %add29 to i64
  %arrayidx34 = getelementptr inbounds i32, i32 addrspace(1)* %g, i64 %idxprom33
  store i32 %add32, i32 addrspace(1)* %arrayidx34, align 4
  br label %if.end35

if.end35:                                         ; preds = %if.then24, %if.end
  ret void
}

define internal i64 @_Z12get_group_idj(i32 %x) {
entry:
  %call = tail call i64 @__mux_get_group_id(i32 %x)
  ret i64 %call
}

define internal i64 @_Z12get_local_idj(i32 %x) {
entry:
  %call = tail call i64 @__mux_get_local_id(i32 %x)
  ret i64 %call
}

define internal i32 @_Z3minii(i32 %x, i32 %y) {
entry:
  %0 = tail call i32 @llvm.smin.i32(i32 %x, i32 %y)
  ret i32 %0
}

declare void @__mux_work_group_barrier(i32, i32, i32)

declare i32 @llvm.smin.i32(i32, i32)

declare i64 @__mux_get_local_id(i32)

declare i64 @__mux_get_group_id(i32)

declare i64 @__mux_get_local_size(i32)

declare void @__mux_set_local_id(i32, i64)

attributes #0 = { "mux-kernel"="entry-point" }
