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

; RUN: muxc --passes barriers-pass,verify -S %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

; makes sure there's an unconditional branch between the subkernels

; CHECK: void @barrier_cfg_linear.mux-barrier-wrapper(
; CHECK: entry:
; CHECK: br label %sw.bb1

; CHECK: sw.bb1:
; CHECK: call {{.*}}i32 @__vecz_v16_barrier_cfg_linear.mux-barrier-region(

; it doesn't need to store the result
; CHECK-NOT: store
; CHECK: exitIR{{[0-9]*}}:
; CHECK: exitIR{{[0-9]*}}:
; CHECK: exitIR{{[0-9]*}}:

; branch directly to the 2nd kernel loop
; CHECK-NEXT: br label %sw.bb2

; CHECK: sw.bb2:
; CHECK: call void @__mux_mem_barrier(i32 1, i32 272)
; CHECK: call {{.*}}i32 @__vecz_v16_barrier_cfg_linear.mux-barrier-region.1(

; it doesn't need to store the result
; CHECK-NOT: store
; CHECK: exitIR{{[0-9]*}}:
; CHECK: exitIR{{[0-9]*}}:
; CHECK: exitIR{{[0-9]*}}:

; branch directly to the exit block
; CHECK: br label %kernel.exit
; CHECK: kernel.exit:
; CHECK: ret void

define internal void @barrier_cfg_linear(i32 addrspace(1)* %d, i32 addrspace(1)* %a, i32 addrspace(1)* %b) !reqd_work_group_size !12 !codeplay_ca_vecz.base !14 {
entry:
  %call = tail call i64 @__mux_get_global_id(i32 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %a, i64 %call
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %b, i64 %call
  %1 = load i32, i32 addrspace(1)* %arrayidx1, align 4
  %add = add nsw i32 %1, %0
  tail call void @__mux_work_group_barrier(i32 0, i32 1, i32 272)
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %call
  store i32 %add, i32 addrspace(1)* %arrayidx2, align 4
  ret void
}

declare void @__mux_work_group_barrier(i32, i32, i32)

define void @__vecz_v16_barrier_cfg_linear(i32 addrspace(1)* %d, i32 addrspace(1)* %a, i32 addrspace(1)* %b) #0 !reqd_work_group_size !12 !codeplay_ca_vecz.derived !21 {
entry:
  %call = tail call i64 @__mux_get_global_id(i32 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %a, i64 %call
  %tmp.a = bitcast i32 addrspace(1)* %arrayidx to <16 x i32> addrspace(1)*
  %0 = load <16 x i32>, <16 x i32> addrspace(1)* %tmp.a, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %b, i64 %call
  %tmp.b = bitcast i32 addrspace(1)* %arrayidx1 to <16 x i32> addrspace(1)*
  %1 = load <16 x i32>, <16 x i32> addrspace(1)* %tmp.b, align 4
  %add1 = add nsw <16 x i32> %1, %0
  tail call void @__mux_work_group_barrier(i32 0, i32 1, i32 272)
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %call
  %tmp.d = bitcast i32 addrspace(1)* %arrayidx2 to <16 x i32> addrspace(1)*
  store <16 x i32> %add1, <16 x i32> addrspace(1)* %tmp.d, align 4
  ret void
}

declare i64 @__mux_get_global_id(i32)

declare i64 @__mux_get_local_size(i32)

declare i64 @__mux_get_group_id(i32)

declare i64 @__mux_get_local_id(i32)

declare i64 @__mux_get_global_offset(i32)

declare void @__mux_set_local_id(i32, i64)

attributes #0 = { "mux-kernel"="entry-point" "mux-base-fn-name"="barrier_cfg_linear" }

!12 = !{i32 64, i32 64, i32 64}
!14 = !{!15, void (i32 addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*)* @__vecz_v16_barrier_cfg_linear}
!15 = !{i32 16, i32 0, i32 0, i32 0}
!16 = !{!17, !17, i64 0}
!17 = !{!"int", !18, i64 0}
!18 = !{!"omnipotent char", !19, i64 0}
!19 = !{!"Simple C/C++ TBAA"}
!21 = !{!15, void (i32 addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*)* @barrier_cfg_linear}
