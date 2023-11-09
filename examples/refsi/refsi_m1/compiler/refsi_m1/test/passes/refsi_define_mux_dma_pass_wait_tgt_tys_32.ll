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

; REQUIRES: llvm-17+
; RUN: muxc --device "%riscv_device" %s --passes replace-target-ext-tys,define-mux-dma,verify -S | FileCheck %s

; Note - RefSi M1 is a 64-bit architecture. This test tests that the compiler
; passes would correctly generate code for a theoretical 32-bit version of this
; architecture.
target datalayout = "e-m:e-p:32:32-i64:64-i128:128-n64-S128"
target triple = "riscv32-unknown-unknown-elf"

declare spir_func void @__mux_dma_wait(i32, ptr)

; CHECK: define spir_func void @__refsi_dma_wait(i32 [[numEvents:%.*]], ptr [[eventList:%.*]]) #0 {
; CHECK:   %loop_iv = phi i32 [ 0, %entry ], [ %new_iv, %body ]
; CHECK:   %max_xfer_id = phi i32 [ 0, %entry ], [ %new_max_xfer_id, %body ]
; CHECK:   [[eventGep:%.*]] = getelementptr i32, ptr [[eventList]], i32 %loop_iv
; CHECK:   %xfer_id = load i32, ptr [[eventGep]], align 4
; CHECK:   %new_iv = add i32 %loop_iv, 1
; CHECK:   [[higher:%.*]] = icmp ugt i32 %xfer_id, %max_xfer_id
; CHECK:   %new_max_xfer_id = select i1 [[higher]], i32 %xfer_id, i32 %max_xfer_id
; CHECK:   %exit_cond = icmp ult i32 %new_iv, [[numEvents]]
; CHECK:   br i1 %exit_cond, label %body, label %epilog
; CHECK:   %event_id_to_wait = phi i32 [ 0, %entry ], [ %new_max_xfer_id, %body ]
; CHECK:   [[store:%.*]] = zext i32 %event_id_to_wait to i64
; CHECK:   store volatile i64 [[store]], ptr inttoptr (i64 536879120 to ptr), align 8
