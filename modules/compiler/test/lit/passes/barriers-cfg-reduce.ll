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

; RUN: muxc --passes replace-wgc,prepare-barriers,barriers-pass,verify -S %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

; makes sure the accumulator initialization is not inside a work-item loop

; CHECK: i32 @reduction.mux-barrier-region.1(
; CHECK: store i32 0, {{(i32 addrspace\(3\)\*)|(ptr addrspace\(3\))}} @__mux_work_group_reduce_add_i32.accumulator

; CHECK: void @reduction.mux-barrier-wrapper(
; CHECK-LABEL: sw.bb2:
; CHECK: call void @__mux_mem_barrier(i32 2, i32 272)
; CHECK: call i32 @reduction.mux-barrier-region.1(

; CHECK: br label %sw.bb3

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
