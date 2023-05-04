; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --device "%riscv_device" %s --passes define-mux-dma,verify -S | %filecheck %s

target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
target triple = "riscv64-unknown-unknown-elf"

%__mux_dma_event_t = type opaque
declare spir_func void @__mux_dma_wait(i32, %__mux_dma_event_t**)


; CHECK: define spir_func void @__refsi_dma_wait(i32 [[numEvents:%.*]], ptr [[eventList:%.*]]) #0 {
; CHECK:   %loop_iv = phi i32 [ 0, %entry ], [ %new_iv, %body ]
; CHECK:   %max_xfer_id = phi i32 [ 0, %entry ], [ %new_max_xfer_id, %body ]
; CHECK:   [[eventGep:%.*]] = getelementptr ptr, ptr [[eventList]], i32 %loop_iv
; CHECK:   [[event:%.*]] = load ptr, ptr [[eventGep]], align 8
; CHECK:   %xfer_id = ptrtoint ptr [[event]] to i32
; CHECK:   %new_iv = add i32 %loop_iv, 1
; CHECK:   [[higher:%.*]] = icmp ugt i32 %xfer_id, %max_xfer_id
; CHECK:   %new_max_xfer_id = select i1 [[higher]], i32 %xfer_id, i32 %max_xfer_id
; CHECK:   %exit_cond = icmp ult i32 %new_iv, [[numEvents]]
; CHECK:   br i1 %exit_cond, label %body, label %epilog
; CHECK:   %event_id_to_wait = phi i32 [ 0, %entry ], [ %new_max_xfer_id, %body ]
; CHECK:   [[store:%.*]] = zext i32 %event_id_to_wait to i64
; CHECK:   store volatile i64 [[store]], ptr inttoptr (i64 536879120 to ptr), align 8
