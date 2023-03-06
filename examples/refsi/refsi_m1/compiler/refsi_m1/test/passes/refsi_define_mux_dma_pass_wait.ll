; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %muxc --device "%riscv_device" %s --passes define-mux-dma,verify -S | %filecheck %t

target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
target triple = "riscv64-unknown-unknown-elf"

%__mux_dma_event_t = type opaque
declare spir_func void @__mux_dma_wait(i32, %__mux_dma_event_t**)

; CHECK-LT15: define spir_func void @__refsi_dma_wait(i32 [[numEvents:%.*]], %__mux_dma_event_t** [[eventList:%.*]]) #0 {
; CHECK-LT15:   %loop_iv = phi i32 [ 0, %entry ], [ %new_iv, %body ]
; CHECK-LT15:   %max_xfer_id = phi i32 [ 0, %entry ], [ %new_max_xfer_id, %body ]
; CHECK-LT15:   [[eventGep:%.*]] = getelementptr %__mux_dma_event_t*, %__mux_dma_event_t** [[eventList]], i32 %loop_iv
; CHECK-LT15:   [[event:%.*]] = load %__mux_dma_event_t*, %__mux_dma_event_t** [[eventGep]], align 8
; CHECK-LT15:   %xfer_id = ptrtoint %__mux_dma_event_t* [[event]] to i32
; CHECK-LT15:   %new_iv = add i32 %loop_iv, 1
; CHECK-LT15:   [[higher:%.*]] = icmp ugt i32 %xfer_id, %max_xfer_id
; CHECK-LT15:   %new_max_xfer_id = select i1 [[higher]], i32 %xfer_id, i32 %max_xfer_id
; CHECK-LT15:   %exit_cond = icmp ult i32 %new_iv, [[numEvents]]
; CHECK-LT15:   %event_id_to_wait = phi i32 [ 0, %entry ], [ %new_max_xfer_id, %body ]
; CHECK-LT15:   [[store:%.*]] = zext i32 %event_id_to_wait to i64
; CHECK-LT15:   store volatile i64 [[store]], i64* inttoptr (i64 536879120 to i64*), align 8

; CHECK-GE15: define spir_func void @__refsi_dma_wait(i32 [[numEvents:%.*]], ptr [[eventList:%.*]]) #0 {
; CHECK-GE15:   %loop_iv = phi i32 [ 0, %entry ], [ %new_iv, %body ]
; CHECK-GE15:   %max_xfer_id = phi i32 [ 0, %entry ], [ %new_max_xfer_id, %body ]
; CHECK-GE15:   [[eventGep:%.*]] = getelementptr ptr, ptr [[eventList]], i32 %loop_iv
; CHECK-GE15:   [[event:%.*]] = load ptr, ptr [[eventGep]], align 8
; CHECK-GE15:   %xfer_id = ptrtoint ptr [[event]] to i32
; CHECK-GE15:   %new_iv = add i32 %loop_iv, 1
; CHECK-GE15:   [[higher:%.*]] = icmp ugt i32 %xfer_id, %max_xfer_id
; CHECK-GE15:   %new_max_xfer_id = select i1 [[higher]], i32 %xfer_id, i32 %max_xfer_id
; CHECK-GE15:   %exit_cond = icmp ult i32 %new_iv, [[numEvents]]
; CHECK-GE15:   br i1 %exit_cond, label %body, label %epilog
; CHECK-GE15:   %event_id_to_wait = phi i32 [ 0, %entry ], [ %new_max_xfer_id, %body ]
; CHECK-GE15:   [[store:%.*]] = zext i32 %event_id_to_wait to i64
; CHECK-GE15:   store volatile i64 [[store]], ptr inttoptr (i64 536879120 to ptr), align 8
