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

; RUN: %muxc --device "%riscv_device" %s --passes refsi-wg-loops,verify -S | %filecheck %s

target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
target triple = "riscv64-unknown-unknown-elf"

define void @add(ptr nocapture readonly %args, ptr nocapture readonly %wi, ptr nocapture readonly %wg) #0 !mux_scheduled_fn !0 {
  ret void
}

; CHECK: define void @add.refsi-wg-loop-wrapper(
; CHECK:      call void @add(ptr nocapture readonly %args, ptr nocapture readonly %wi, ptr nocapture readonly %wg) #0
; CHECK-NEXT: call void @__mux_work_group_barrier(i32 -1, i32 2, i32 264)

attributes #0 = { "mux-kernel"="entry-point" }

!mux-scheduling-params = !{!1}


!0 = !{i32 1, i32 2}
!1 = !{!"MuxWorkItemInfo", !"MuxWorkGroupInfo"}
