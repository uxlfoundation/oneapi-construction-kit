; Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
