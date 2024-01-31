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

; RUN: muxc --passes work-item-loops,verify < %s | FileCheck %s

target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "spir64-unknown-unknown"

define internal void @sub_group_all_builtin(ptr addrspace(1) %in, ptr addrspace(1) %out_b) #0 !reqd_work_group_size !11 !codeplay_ca_vecz.base !12 {
entry:
  %call = tail call i64 @__mux_get_global_linear_id() #4
  %arrayidx15 = getelementptr inbounds i32, ptr addrspace(1) %in, i64 %call
  %0 = load i32, ptr addrspace(1) %arrayidx15, align 4
  %1 = icmp ne i32 %0, 0
  %2 = call i1 @__mux_work_group_all_i1(i32 0, i1 %1)
  %arrayidx17 = getelementptr inbounds i1, ptr addrspace(1) %out_b, i64 %call
  store i1 %2, ptr addrspace(1) %arrayidx17, align 4
  ret void
}

define void @__vecz_v32_sub_group_all_builtin(ptr addrspace(1) %in, ptr addrspace(1) %out_b) #3 !reqd_work_group_size !11 !codeplay_ca_vecz.derived !20 {
entry:
  %call = tail call i64 @__mux_get_global_linear_id() #4
  %arrayidx15 = getelementptr inbounds i32, ptr addrspace(1) %in, i64 %call
  %0 = load <32 x i32>, ptr addrspace(1) %arrayidx15, align 4
  %1 = icmp eq <32 x i32> %0, zeroinitializer
  %2 = bitcast <32 x i1> %1 to i32
  %3 = icmp eq i32 %2, 0
  %4 = call i1 @__mux_work_group_all_i1(i32 0, i1 %3)
  %arrayidx17 = getelementptr inbounds i1, ptr addrspace(1) %out_b, i64 %call
  store i1 %4, ptr addrspace(1) %arrayidx17, align 4
  ret void
}

declare i64 @__mux_get_global_linear_id() #1
declare i1 @__mux_work_group_all_i1(i32, i1) #2

attributes #0 = { convergent norecurse nounwind "mux-degenerate-subgroups" "mux-orig-fn"="sub_group_all_builtin" "uniform-work-group-size"="false" }
attributes #1 = { alwaysinline norecurse nounwind "vecz-mode"="auto" }
attributes #2 = { alwaysinline convergent norecurse nounwind }
attributes #3 = { convergent norecurse nounwind "mux-base-fn-name"="__vecz_v64_sub_group_all_builtin" "mux-degenerate-subgroups" "mux-kernel"="entry-point" "mux-orig-fn"="sub_group_all_builtin" "uniform-work-group-size"="false" }
attributes #4 = { alwaysinline norecurse nounwind  }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 3, i32 0}
!11 = !{i32 67, i32 1, i32 1}
!12 = !{!13, ptr @__vecz_v32_sub_group_all_builtin}
!13 = !{i32 32, i32 0, i32 0, i32 0}
!20 = !{!13, ptr @sub_group_all_builtin}

; CHECK-LABEL: sw.bb2:
; CHECK: br label %loopIR13

; CHECK-LABEL: loopIR13:
; CHECK: %[[PHI:.+]] = phi i64 [ 0, %sw.bb2 ], [ %[[INC:.+]], %loopIR13 ]
; CHECK: %[[PHI_ACCUM:.+]] = phi i1 [ true, %sw.bb2 ], [ %[[ACCUM:.+]], %loopIR13 ]
; CHECK: %[[BARRIER:.+]] = getelementptr inbounds %__vecz_v32_sub_group_all_builtin_live_mem_info, ptr %live_variables, i64 %[[PHI]]
; CHECK: %[[ITEM:.+]] = getelementptr inbounds %__vecz_v32_sub_group_all_builtin_live_mem_info, ptr %[[BARRIER]], i32 0, i32 0
; CHECK: %[[LD:.+]] = load i1, ptr %[[ITEM]], align 1
; CHECK: %[[ACCUM]] = and i1 %[[PHI_ACCUM]], %[[LD]]
; CHECK: %[[CMP:.+]] = icmp ult i64 %[[INC]], 2
; CHECK: br i1 %[[CMP]], label %loopIR13, label %exitIR14

; CHECK-LABEL: exitIR14:
; CHECK: %WGC_reduce = phi i1 [ %[[ACCUM]], %loopIR13 ]
