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

; RUN: %muxc --device "%default_device" --passes add-entry-hook,verify -S %s  | %filecheck %s

target triple = "spir32-unknown-unknown"
target datalayout = "e-p:32:32:32-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: define void @foo.host-entry-hook(ptr %wi-info, ptr %sched-info, ptr %mini-wg-info) {{#[0-9]+}} !mux_scheduled_fn {{\![0-9]+}} {
; CHECK-LABEL: entry:
; Just check that the number of groups uses the right size_t for this device.
; Assume the rest of of the code has been checked by the 64-bit version of this
; test.
; CHECK: {{%.*}} = call i32 @__mux_get_num_groups(i32 0, ptr %wi-info, ptr %sched-info, ptr %mini-wg-info)
define void @foo(ptr %wi-info, ptr %sched-info, ptr %mini-wg-info) #0 !mux_scheduled_fn !1 {
  ret void
}

attributes #0 = { "mux-kernel"="entry-point" }

!mux-scheduling-params = !{!0}

!0 = !{!"MuxWorkItemInfo", !"Mux_schedule_info_s", !"MiniWGInfo"}
!1 = !{i32 0, i32 1, i32 2}
