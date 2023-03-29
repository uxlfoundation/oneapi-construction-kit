; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --device "%riscv_device" %s --passes refsi-wg-loops,verify -S -opaque-pointers | %filecheck %s

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
