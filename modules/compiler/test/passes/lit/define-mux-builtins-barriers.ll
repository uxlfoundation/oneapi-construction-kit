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

; RUN: %muxc --passes define-mux-builtins,verify -S %s  | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

declare void @__mux_mem_barrier(i32, i32)
; CHECK: define internal void @__mux_mem_barrier(i32 [[MEM:%.*]], i32 [[SEM:%.*]])
; CHECK: [[VAL:%.*]] = and i32 [[SEM]], 31
; CHECK: switch i32 [[VAL]], label %[[BB_DEFAULT:.*]] [
; CHECK:   i32 2,  label %[[BB_ACQUIRE:.*]]
; CHECK:   i32 4,  label %[[BB_RELEASE:.*]]
; CHECK:   i32 8,  label %[[BB_ACQ_REL:.*]]
; CHECK:   i32 16, label %[[BB_SEQ_CST:.*]]
; CHECK:      [[BB_DEFAULT]]:
; CHECK-NEXT:   br label %[[BB_EXIT:.*]]
; CHECK:      [[BB_ACQUIRE]]:
; CHECK-NEXT:   fence syncscope("singlethread") acquire
; CHECK:      [[BB_RELEASE]]:
; CHECK-NEXT:   fence syncscope("singlethread") release
; CHECK:      [[BB_ACQ_REL]]:
; CHECK-NEXT:   fence syncscope("singlethread") acq_rel
; CHECK:      [[BB_SEQ_CST]]:
; CHECK-NEXT:   fence syncscope("singlethread") seq_cst
; CHECK:      [[BB_EXIT]]:
; CHECK-NEXT:   ret void
