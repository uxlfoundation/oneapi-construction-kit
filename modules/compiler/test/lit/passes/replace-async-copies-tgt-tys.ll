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

; In LLVM 17, OpenCL's "event_t" type is represented by the target extension
; type target("spirv.Event"). The replace-async-copies pass expects that the
; event type has already been replaced with the target's desired event type.
;
; For this we usually run the replace-target-ext-tys-pass beforehand, to
; replace the target extension type with the default event type in ComputeMux
; (i32). A target would also have control over this process.
;
; We also run the pass on the target extension types directly, to simulate a
; target using their own target extension types. This is perfectly valid from a
; ComputeMux point of view.

; REQUIRES: llvm-17+
; RUN: muxc --passes replace-async-copies,verify %s | FileCheck %s -DEVENT_TY='target("spirv.Event")'
; RUN: muxc --passes replace-target-ext-tys,replace-async-copies,verify %s | FileCheck %s -DEVENT_TY=i32

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

declare spir_func void @_Z17wait_group_eventsiP9ocl_event(i32, ptr)
; CHECK-LABEL: define spir_func void @_Z17wait_group_eventsiP9ocl_event(
; CHECK:  call spir_func void @__mux_dma_wait(i32 %0, ptr %1)
; CHECK:  ret void
; CHECK: }

; Also test generic address spaces, which can come out of SPIR-V's OpGroupWaitEvents.
declare spir_func void @_Z17wait_group_eventsiPU3AS49ocl_event(i32 %n, ptr addrspace(4) %events)
; CHECK-LABEL: define spir_func void @_Z17wait_group_eventsiPU3AS49ocl_event(i32 %n, ptr addrspace(4) %events
; CHECK:  [[MUX:%.*]] = addrspacecast ptr addrspace(4) %events to ptr
; CHECK:  call spir_func void @__mux_dma_wait(i32 %n, ptr [[MUX]])
; CHECK:  ret void
; CHECK: }

declare spir_func target("spirv.Event") @_Z21async_work_group_copyPU3AS3iPU3AS1Kim9ocl_event(ptr addrspace(3), ptr addrspace(1), i64, target("spirv.Event"))
; CHECK: define spir_func [[EVENT_TY]] @_Z21async_work_group_copyPU3AS3iPU3AS1Kim9ocl_event(
; CHECK:   %width.bytes = mul i64 4, %2
; CHECK:   %mux.out.event = call spir_func [[EVENT_TY]] @__mux_dma_read_1D(
; CHECK-SAME:  ptr addrspace(3) %0, ptr addrspace(1) %1, i64 %width.bytes, [[EVENT_TY]] %3)
; CHECK:   ret [[EVENT_TY]] %mux.out.event
; CHECK: }

declare spir_func target("spirv.Event") @_Z21async_work_group_copyPU3AS3Dv4_iPU3AS1KS_m9ocl_event(ptr addrspace(3), ptr addrspace(1), i64, target("spirv.Event"))
; CHECK: define spir_func [[EVENT_TY]] @_Z21async_work_group_copyPU3AS3Dv4_iPU3AS1KS_m9ocl_event(
; CHECK:   %width.bytes = mul i64 16, %2
; CHECK:   %mux.out.event = call spir_func [[EVENT_TY]] @__mux_dma_read_1D(
; CHECK-SAME:  ptr addrspace(3) %0, ptr addrspace(1) %1, i64 %width.bytes, [[EVENT_TY]] %3)
; CHECK:   ret [[EVENT_TY]] %mux.out.event
; CHECK: }

declare spir_func target("spirv.Event") @_Z26async_work_group_copy_2D2DPU3AS3vmPU3AS1Kvmmmmmm9ocl_event(ptr addrspace(3), i64, ptr addrspace(1), i64, i64, i64, i64, i64, i64, target("spirv.Event"))
; CHECK: define spir_func [[EVENT_TY]] @_Z26async_work_group_copy_2D2DPU3AS3vmPU3AS1Kvmmmmmm9ocl_event(
; CHECK:  %10 = mul i64 %1, %4
; CHECK:  %11 = mul i64 %3, %4
; CHECK:  %12 = mul i64 %5, %4
; CHECK:  %13 = getelementptr i8, ptr addrspace(3) %0, i64 %10
; CHECK:  %14 = getelementptr i8, ptr addrspace(1) %2, i64 %11
; CHECK:  %15 = mul i64 %7, %4
; CHECK:  %16 = mul i64 %8, %4
; CHECK:  %17 = call spir_func [[EVENT_TY]] @__mux_dma_read_2D(ptr addrspace(3) %13, ptr addrspace(1) %14,
; CHECK-SAME:  i64 %12, i64 %16, i64 %15, i64 %6, [[EVENT_TY]] %9)
; CHECK:  ret [[EVENT_TY]] %17
; CHECK: }

declare spir_func target("spirv.Event") @_Z26async_work_group_copy_3D3DPU3AS3vmPU3AS1Kvmmmmmmmmm9ocl_event(ptr addrspace(3), i64, ptr addrspace(1), i64, i64, i64, i64, i64, i64, i64, i64, i64, target("spirv.Event"))
; CHECK: define spir_func [[EVENT_TY]] @_Z26async_work_group_copy_3D3DPU3AS3vmPU3AS1Kvmmmmmmmmm9ocl_event(
; CHECK:  %13 = mul i64 %1, %4
; CHECK:  %14 = mul i64 %3, %4
; CHECK:  %15 = mul i64 %5, %4
; CHECK:  %16 = getelementptr i8, ptr addrspace(3) %0, i64 %13
; CHECK:  %17 = getelementptr i8, ptr addrspace(1) %2, i64 %14
; CHECK:  %18 = mul i64 %8, %4
; CHECK:  %19 = mul i64 %10, %4
; CHECK:  %20 = mul i64 %9, %4
; CHECK:  %21 = mul i64 %11, %4
; CHECK:  %22 = call spir_func [[EVENT_TY]] @__mux_dma_read_3D(ptr addrspace(3) %16, ptr addrspace(1) %17,
; CHECK-SAME:  i64 %15, i64 %19, i64 %18, i64 %6, i64 %21, i64 %20, i64 %7, [[EVENT_TY]] %12)
; CHECK:  ret [[EVENT_TY]] %22
; CHECK: }
