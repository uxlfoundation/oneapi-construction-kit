; Copyright (C) Codeplay Software Limited
;
; Licensed under the Apache License, Version 2.0 (the "License") with LLVM
; Exceptions; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

; RUN: muxc --passes work-item-loops,verify -S %s | FileCheck %s

; It checks to make sure that the variable used by the barrier is reloaded from
; the barrier struct for use in the newly-created Memory Barrier call in the
; wrapper function.

; CHECK-LABEL: sw.bb2:
; CHECK-NEXT: %[[GEP:.+]] = getelementptr inbounds %test_fence_live_mem_info, ptr %live_variables, i64 0
; CHECK-NEXT: %live_gep_secret = getelementptr inbounds %test_fence_live_mem_info, ptr %[[GEP]], i32 0, i32 0
; CHECK-NEXT: %secret_load = load i32, ptr %live_gep_secret, align 4
; CHECK-NEXT: call void @__mux_mem_barrier(i32 %secret_load, i32 912)

; ModuleID = 'SPIR-V'
source_filename = "SPIR-V"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-unknown-elf"

; Function Attrs: convergent nounwind
define internal void @test_fence(ptr addrspace(3) nocapture %out) local_unnamed_addr #0 {
entry:
  call void @mysterious_side_effect(ptr addrspace(3) %out)
  %secret = call i32 @mysterious_secret_generator(i32 0)
  tail call void @__mux_work_group_barrier(i32 0, i32 %secret, i32 912)
  call void @mysterious_side_effect(ptr addrspace(3) %out)
  ret void
}

declare i32 @mysterious_secret_generator(i32)
declare void @mysterious_side_effect(ptr addrspace(3))
declare void @__mux_work_group_barrier(i32, i32, i32)

attributes #0 = { "mux-kernel"="entry-point" }
