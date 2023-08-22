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

; RUN: muxc --passes prepare-barriers,barriers-pass,verify -S %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

; makes sure the accumulator initialization is not inside a work-item loop

; CHECK: void @reduction.mux-barrier-wrapper(
; CHECK-LABEL: sw.bb2:
; CHECK: br label %[[REDUCE_LOOP:.+]]

; CHECK: [[REDUCE_LOOP]]:
; CHECK:  %[[IDX:.+]] = phi i64 [ 0, %sw.bb2 ], [ %[[IDX_NEXT:.+]], %[[REDUCE_LOOP]] ]
; CHECK:  %[[ACCUM:.+]] = phi i32 [ 0, %sw.bb2 ], [ %[[ACCUM_NEXT:.+]], %[[REDUCE_LOOP]] ]
; CHECK:  %[[ITEM:.+]] = getelementptr inbounds %reduction_live_mem_info, {{ptr|.+\*}} %live_variables, i64 %[[IDX]]
; CHECK:  %[[VAL:.+]] = getelementptr inbounds %reduction_live_mem_info, {{ptr|.+\*}} %[[ITEM]], i32 0, i32 0
; CHECK:  %[[LD:.+]] = load i32, {{ptr|.+\*}} %[[VAL]], align 4
; CHECK:  %[[ACCUM_NEXT]] = add i32 %[[ACCUM]], %[[LD]]
; CHECK:  %[[IDX_NEXT]] = add i64 %[[IDX]], 1
; CHECK:  %[[LOOP_COND:.+]] = icmp ult i64 %24, 262144
; CHECK:  br i1 %[[LOOP_COND]], label %[[REDUCE_LOOP]], label %[[REDUCE_EXIT:[^,]+]]

; CHECK: [[REDUCE_EXIT]]:
; CHECK:  %reduce = phi i32 [ %23, %[[REDUCE_LOOP]] ]

declare i64 @__mux_get_global_id(i32 %x)
declare i32 @__mux_work_group_reduce_add_i32(i32 %id, i32 %x)

define internal void @reduction(i32 addrspace(1)* %d, i32 addrspace(1)* %a) #0 !reqd_work_group_size !0 {
entry:
  %call = tail call i64 @__mux_get_global_id(i32 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %a, i64 %call
  %ld = load i32, i32 addrspace(1)* %arrayidx, align 4
  %reduce = call i32 @__mux_work_group_reduce_add_i32(i32 0, i32 %ld)
  store i32 %reduce, i32 addrspace(1)* %d, align 4
  ret void
}

attributes #0 = { "mux-kernel"="entry-point" }

!opencl.ocl.version = !{!1}

!0 = !{i32 64, i32 64, i32 64}
!1 = !{i32 3, i32 0}
