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

target triple = "spir32-unknown-unknown"
target datalayout = "e-i64:64-p:32:32-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

; CHECK: %minimal_barrier_2_live_mem_info = type { i32, i32 }

@minimal_barrier_2.cache = internal unnamed_addr addrspace(3) global [4 x i32] undef, align 4

define void @minimal_barrier_2(i32 addrspace(1)* %output) #0 {
if.end:
  %call1 = tail call i32 @_Z12get_local_idj(i32 0)
  %arrayidx = getelementptr inbounds [4 x i32], [4 x i32] addrspace(3)* @minimal_barrier_2.cache, i32 0, i32 %call1
  br label %for.body9

for.cond.cleanup:                                 ; preds = %if.then17, %for.cond.cleanup8
  %my_value.3 = phi i32 [ %4, %if.then17 ], [ %add13, %for.cond.cleanup8 ]
  %call25 = tail call i32 @_Z13get_global_idj(i32 0)
  %cmp26 = icmp eq i32 %call25, 1
  br i1 %cmp26, label %if.then27, label %if.end29

for.cond.cleanup8:                                ; preds = %for.body9
  %call14 = tail call i32 @_Z12get_local_idj(i32 0)
  %cmp16 = icmp ugt i32 %call14, %call6
  br i1 %cmp16, label %if.then17, label %for.cond.cleanup

for.body9:                                        ; preds = %for.body9, %if.end
  %j.047 = phi i32 [ %inc, %for.body9 ], [ 0, %if.end ]
  %my_value.246 = phi i32 [ %add13, %for.body9 ], [ 0, %if.end ]
  store i32 %call1, i32 addrspace(3)* %arrayidx, align 4
  tail call void @__mux_work_group_barrier(i32 0, i32 1, i32 272)
  %0 = load i32, i32 addrspace(3)* getelementptr inbounds ([4 x i32], [4 x i32] addrspace(3)* @minimal_barrier_2.cache, i32 0, i32 0), align 4
  %add = add nsw i32 %0, %my_value.246
  %1 = load i32, i32 addrspace(3)* getelementptr inbounds ([4 x i32], [4 x i32] addrspace(3)* @minimal_barrier_2.cache, i32 0, i32 1), align 4
  %add11 = add nsw i32 %add, %1
  %2 = load i32, i32 addrspace(3)* getelementptr inbounds ([4 x i32], [4 x i32] addrspace(3)* @minimal_barrier_2.cache, i32 0, i32 2), align 4
  %add12 = add nsw i32 %add11, %2
  %3 = load i32, i32 addrspace(3)* getelementptr inbounds ([4 x i32], [4 x i32] addrspace(3)* @minimal_barrier_2.cache, i32 0, i32 3), align 4
  %add13 = add nsw i32 %add12, %3
  %inc = add nuw nsw i32 %j.047, 1
  %call6 = tail call i32 @_Z14get_local_sizej(i32 0)
  %cmp7 = icmp ult i32 %inc, %call6
  br i1 %cmp7, label %for.body9, label %for.cond.cleanup8

if.then17:                                        ; preds = %for.cond.cleanup8
  %arrayidx19 = getelementptr inbounds [4 x i32], [4 x i32] addrspace(3)* @minimal_barrier_2.cache, i32 0, i32 %call14
  %4 = load i32, i32 addrspace(3)* %arrayidx19, align 4
  br label %for.cond.cleanup

if.then27:                                        ; preds = %for.cond.cleanup
  store i32 %my_value.3, i32 addrspace(1)* %output, align 4
  br label %if.end29

if.end29:                                         ; preds = %if.then27, %for.cond.cleanup
  ret void
}

define internal i32 @_Z13get_global_idj(i32 %x) {
entry:
  %call = tail call i32 @__mux_get_global_id(i32 %x)
  ret i32 %call
}

define internal i32 @_Z12get_local_idj(i32 %x) {
entry:
  %call = tail call i32 @__mux_get_local_id(i32 %x)
  ret i32 %call
}

define internal i32 @_Z14get_local_sizej(i32 %x) {
entry:
  %call = tail call i32 @__mux_get_local_size(i32 %x)
  ret i32 %call
}

declare void @__mux_work_group_barrier(i32, i32, i32)

declare i32 @__mux_get_global_id(i32)

declare i32 @__mux_get_local_id(i32)

declare i32 @__mux_get_local_size(i32)

declare i32 @__mux_get_group_id(i32)

declare i32 @__mux_get_global_offset(i32)

declare void @__mux_set_local_id(i32, i32)

attributes #0 = { "mux-kernel"="entry-point" }
