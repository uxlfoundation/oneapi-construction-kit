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

; RUN: muxc --passes replace-async-copies,verify -S %s  | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; It's possible for a module using opaque pointers to have no %opencl.event_t
; type definitions, since they're never seen to be used by any functions. We
; must trust the compiler to generate the definition on demand, using opaque
; structs.

declare spir_func ptr @_Z21async_work_group_copyPU3AS3iPU3AS1Kim9ocl_event(i32 addrspace(3)*, i32 addrspace(1)*, i64, ptr)
; CHECK-LABEL: define spir_func ptr @_Z21async_work_group_copyPU3AS3iPU3AS1Kim9ocl_event(
; CHECK:   %width.bytes = mul i64 4, %2
; CHECK:   %mux.out.event = call spir_func ptr @__mux_dma_read_1D(ptr addrspace(3) %0, ptr addrspace(1) %1, i64 %width.bytes, ptr %3)
; CHECK:   ret ptr %mux.out.event

; CHECK: }

declare spir_func ptr @_Z21async_work_group_copyPU3AS3Dv4_iPU3AS1KS_m9ocl_event(<4 x i32> addrspace(3)*, <4 x i32> addrspace(1)*, i64, ptr)
; CHECK-LABEL: define spir_func ptr @_Z21async_work_group_copyPU3AS3Dv4_iPU3AS1KS_m9ocl_event(
; CHECK:   %width.bytes = mul i64 16, %2
; CHECK:   %mux.out.event = call spir_func ptr @__mux_dma_read_1D(ptr addrspace(3) %0, ptr addrspace(1) %1, i64 %width.bytes, ptr %3)
; CHECK:   ret ptr %mux.out.event

; CHECK: }

declare spir_func ptr @_Z26async_work_group_copy_2D2DPU3AS3vmPU3AS1Kvmmmmmm9ocl_event(i8 addrspace(3)*, i64, i8 addrspace(1)*, i64, i64, i64, i64, i64, i64, ptr)
; CHECK-LABEL: define spir_func ptr @_Z26async_work_group_copy_2D2DPU3AS3vmPU3AS1Kvmmmmmm9ocl_event(
; CHECK:  %10 = mul i64 %1, %4
; CHECK:  %11 = mul i64 %3, %4
; CHECK:  %12 = mul i64 %5, %4
; CHECK:  %13 = getelementptr i8, ptr addrspace(3) %0, i64 %10
; CHECK:  %14 = getelementptr i8, ptr addrspace(1) %2, i64 %11
; CHECK:  %15 = mul i64 %7, %4
; CHECK:  %16 = mul i64 %8, %4
; CHECK:  %17 = call spir_func ptr @__mux_dma_read_2D(ptr addrspace(3) %13, ptr addrspace(1) %14, i64 %12, i64 %16, i64 %15, i64 %6, ptr %9)
; CHECK:  ret ptr %17

; CHECK: }

declare spir_func ptr @_Z26async_work_group_copy_3D3DPU3AS3vmPU3AS1Kvmmmmmmmmm9ocl_event(i8 addrspace(3)*, i64, i8 addrspace(1)*, i64, i64, i64, i64, i64, i64, i64, i64, i64, ptr)
; CHECK-LABEL: define spir_func ptr @_Z26async_work_group_copy_3D3DPU3AS3vmPU3AS1Kvmmmmmmmmm9ocl_event(
; CHECK:  %13 = mul i64 %1, %4
; CHECK:  %14 = mul i64 %3, %4
; CHECK:  %15 = mul i64 %5, %4
; CHECK:  %16 = getelementptr i8, ptr addrspace(3) %0, i64 %13
; CHECK:  %17 = getelementptr i8, ptr addrspace(1) %2, i64 %14
; CHECK:  %18 = mul i64 %8, %4
; CHECK:  %19 = mul i64 %10, %4
; CHECK:  %20 = mul i64 %9, %4
; CHECK:  %21 = mul i64 %11, %4
; CHECK:  %22 = call spir_func ptr @__mux_dma_read_3D(ptr addrspace(3) %16, ptr addrspace(1) %17, i64 %15, i64 %19, i64 %18, i64 %6, i64 %21, i64 %20, i64 %7, ptr %12)
; CHECK:  ret ptr %22

; CHECK: }

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
