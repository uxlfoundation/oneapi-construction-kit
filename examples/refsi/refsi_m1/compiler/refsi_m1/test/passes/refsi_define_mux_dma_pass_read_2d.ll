; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %muxc --device "%riscv_device" %s --passes define-mux-dma,verify -S | %filecheck %t

target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
target triple = "riscv64-unknown-unknown-elf"

%__mux_dma_event_t = type opaque
declare spir_func %__mux_dma_event_t* @__mux_dma_read_2D(i8 addrspace(3)*, i8 addrspace(1)*, i64, i64, i64, i64, %__mux_dma_event_t*)

; CHECK-LT15: define spir_func %__mux_dma_event_t* @__refsi_dma_start_2d_read(i8 addrspace(3)* [[dstDmaPointer:%.*]], i8 addrspace(1)* [[srcDmaPointer:%.*]], i64 [[width:%.*]], i64 [[dstStride:%.*]], i64 [[srcStride:%.*]], i64 [[height:%.*]], %__mux_dma_event_t* [[event:%.*]]) #0 {
; CHECK-LT15:   [[dst_int:%.*]] = ptrtoint i8 addrspace(3)* [[dstDmaPointer]] to i64
; CHECK-LT15:   store volatile i64 [[dst_int]], i64* inttoptr (i64 536879136 to i64*), align 8
; CHECK-LT15:   [[src_int:%.*]] = ptrtoint i8 addrspace(1)* [[srcDmaPointer]] to i64
; CHECK-LT15:   store volatile i64 [[src_int]], i64* inttoptr (i64 536879128 to i64*), align 8
; CHECK-LT15:   store volatile i64 [[width]], i64* inttoptr (i64 536879144 to i64*), align 8
; CHECK-LT15:   store volatile i64 [[height]], i64* inttoptr (i64 536879152 to i64*), align 8
; CHECK-LT15:   store volatile i64 [[srcStride]], i64* inttoptr (i64 536879168 to i64*), align 8
; CHECK-LT15:   store volatile i64 [[dstStride]], i64* inttoptr (i64 536879184 to i64*), align 8
; CHECK-LT15:   store volatile i64 225, i64* inttoptr (i64 536879104 to i64*), align 8
; CHECK-LT15:   [[load:%.*]] = load volatile i64, i64* inttoptr (i64 536879112 to i64*), align 8

; CHECK-GE15: define spir_func ptr @__refsi_dma_start_2d_read(ptr addrspace(3) [[dstDmaPointer:%.*]], ptr addrspace(1) [[srcDmaPointer:%.*]], i64 [[width:%.*]], i64 [[dstStride:%.*]], i64 [[srcStride:%.*]], i64 [[height:%.*]], ptr [[event:%.*]]) #0 {
; CHECK-GE15:   [[dst_int:%.*]] = ptrtoint ptr addrspace(3) [[dstDmaPointer]] to i64
; CHECK-GE15:   store volatile i64 [[dst_int]], ptr inttoptr (i64 536879136 to ptr), align 8
; CHECK-GE15:   [[src_int:%.*]] = ptrtoint ptr addrspace(1) [[srcDmaPointer]] to i64
; CHECK-GE15:   store volatile i64 [[src_int]], ptr inttoptr (i64 536879128 to ptr), align 8
; CHECK-GE15:   store volatile i64 [[width]], ptr inttoptr (i64 536879144 to ptr), align 8
; CHECK-GE15:   store volatile i64 [[height]], ptr inttoptr (i64 536879152 to ptr), align 8
; CHECK-GE15:   store volatile i64 [[srcStride]], ptr inttoptr (i64 536879168 to ptr), align 8
; CHECK-GE15:   store volatile i64 [[dstStride]], ptr inttoptr (i64 536879184 to ptr), align 8
; CHECK-GE15:   store volatile i64 225, ptr inttoptr (i64 536879104 to ptr), align 8
; CHECK-GE15:   [[load:%.*]] = load volatile i64, ptr inttoptr (i64 536879112 to ptr), align 8

