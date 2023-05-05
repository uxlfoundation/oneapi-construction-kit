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

; RUN: %muxc --device "%riscv_device" %s --passes define-mux-dma,verify -S | %filecheck %s

target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
target triple = "riscv64-unknown-unknown-elf"

%__mux_dma_event_t = type opaque
declare spir_func %__mux_dma_event_t* @__mux_dma_read_2D(i8 addrspace(3)*, i8 addrspace(1)*, i64, i64, i64, i64, %__mux_dma_event_t*)


; CHECK: define spir_func ptr @__refsi_dma_start_2d_read(ptr addrspace(3) [[dstDmaPointer:%.*]], ptr addrspace(1) [[srcDmaPointer:%.*]], i64 [[width:%.*]], i64 [[dstStride:%.*]], i64 [[srcStride:%.*]], i64 [[height:%.*]], ptr [[event:%.*]]) #0 {
; CHECK:   [[dst_int:%.*]] = ptrtoint ptr addrspace(3) [[dstDmaPointer]] to i64
; CHECK:   store volatile i64 [[dst_int]], ptr inttoptr (i64 536879136 to ptr), align 8
; CHECK:   [[src_int:%.*]] = ptrtoint ptr addrspace(1) [[srcDmaPointer]] to i64
; CHECK:   store volatile i64 [[src_int]], ptr inttoptr (i64 536879128 to ptr), align 8
; CHECK:   store volatile i64 [[width]], ptr inttoptr (i64 536879144 to ptr), align 8
; CHECK:   store volatile i64 [[height]], ptr inttoptr (i64 536879152 to ptr), align 8
; CHECK:   store volatile i64 [[srcStride]], ptr inttoptr (i64 536879168 to ptr), align 8
; CHECK:   store volatile i64 [[dstStride]], ptr inttoptr (i64 536879184 to ptr), align 8
; CHECK:   store volatile i64 225, ptr inttoptr (i64 536879104 to ptr), align 8
; CHECK:   [[load:%.*]] = load volatile i64, ptr inttoptr (i64 536879112 to ptr), align 8

