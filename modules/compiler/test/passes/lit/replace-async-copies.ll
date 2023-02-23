; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %muxc --passes replace-async-copies,verify -S %s | %filecheck %t

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

%opencl.event_t = type opaque

declare spir_func %opencl.event_t* @_Z21async_work_group_copyPU3AS3iPU3AS1Kim9ocl_event(i32 addrspace(3)*, i32 addrspace(1)*, i64, %opencl.event_t*)
; CHECK-LABEL-GE15: define spir_func ptr @_Z21async_work_group_copyPU3AS3iPU3AS1Kim9ocl_event(
; CHECK-LABEL-LT15: define spir_func %opencl.event_t* @_Z21async_work_group_copyPU3AS3iPU3AS1Kim9ocl_event(
; CHECK:   %width.bytes = mul i64 4, %2
; CHECK-GE15:   %mux.out.event = call spir_func ptr @__mux_dma_read_1D(ptr addrspace(3) %0, ptr addrspace(1) %1, i64 %width.bytes, ptr %3)
; CHECK-GE15:   ret ptr %mux.out.event

; CHECK-LT15:   %mux.in.event = bitcast %opencl.event_t* %3 to %__mux_dma_event_t*
; CHECK-LT15:   %mux.dst = bitcast i32 addrspace(3)* %0 to i8 addrspace(3)*
; CHECK-LT15:   %mux.src = bitcast i32 addrspace(1)* %1 to i8 addrspace(1)*
; CHECK-LT15:   %mux.out.event = call spir_func %__mux_dma_event_t* @__mux_dma_read_1D(i8 addrspace(3)* %mux.dst, i8 addrspace(1)* %mux.src, i64 %width.bytes, %__mux_dma_event_t* %mux.in.event)
; CHECK-LT15:   %clc.out.event = bitcast %__mux_dma_event_t* %mux.out.event to %opencl.event_t*
; CHECK-LT15:   ret %opencl.event_t* %clc.out.event
; CHECK: }

declare spir_func %opencl.event_t* @_Z21async_work_group_copyPU3AS3Dv4_iPU3AS1KS_m9ocl_event(<4 x i32> addrspace(3)*, <4 x i32> addrspace(1)*, i64, %opencl.event_t*)
; CHECK-LABEL-GE15: define spir_func ptr @_Z21async_work_group_copyPU3AS3Dv4_iPU3AS1KS_m9ocl_event(
; CHECK-LABEL-LT15: define spir_func %opencl.event_t* @_Z21async_work_group_copyPU3AS3Dv4_iPU3AS1KS_m9ocl_event(
; CHECK:   %width.bytes = mul i64 16, %2
; CHECK-GE15:   %mux.out.event = call spir_func ptr @__mux_dma_read_1D(ptr addrspace(3) %0, ptr addrspace(1) %1, i64 %width.bytes, ptr %3)
; CHECK-GE15:   ret ptr %mux.out.event

; CHECK-LT15:   %mux.in.event = bitcast %opencl.event_t* %3 to %__mux_dma_event_t*
; CHECK-LT15:   %mux.dst = bitcast <4 x i32> addrspace(3)* %0 to i8 addrspace(3)*
; CHECK-LT15:   %mux.src = bitcast <4 x i32> addrspace(1)* %1 to i8 addrspace(1)*
; CHECK-LT15:   %mux.out.event = call spir_func %__mux_dma_event_t* @__mux_dma_read_1D(i8 addrspace(3)* %mux.dst, i8 addrspace(1)* %mux.src, i64 %width.bytes, %__mux_dma_event_t* %mux.in.event)
; CHECK-LT15:   %clc.out.event = bitcast %__mux_dma_event_t* %mux.out.event to %opencl.event_t*
; CHECK-LT15:   ret %opencl.event_t* %clc.out.event
; CHECK: }

declare spir_func %opencl.event_t* @_Z26async_work_group_copy_2D2DPU3AS3vmPU3AS1Kvmmmmmm9ocl_event(i8 addrspace(3)*, i64, i8 addrspace(1)*, i64, i64, i64, i64, i64, i64, %opencl.event_t*)
; CHECK-LABEL-GE15: define spir_func ptr @_Z26async_work_group_copy_2D2DPU3AS3vmPU3AS1Kvmmmmmm9ocl_event(
; CHECK-LABEL-LT15: define spir_func %opencl.event_t* @_Z26async_work_group_copy_2D2DPU3AS3vmPU3AS1Kvmmmmmm9ocl_event(
; CHECK-LT15:  %mux.in.event = bitcast %opencl.event_t* %9 to %__mux_dma_event_t*
; CHECK:  %10 = mul i64 %1, %4
; CHECK:  %11 = mul i64 %3, %4
; CHECK:  %12 = mul i64 %5, %4
; CHECK-GE15:  %13 = getelementptr i8, ptr addrspace(3) %0, i64 %10
; CHECK-GE15:  %14 = getelementptr i8, ptr addrspace(1) %2, i64 %11
; CHECK-LT15:  %13 = getelementptr i8, i8 addrspace(3)* %0, i64 %10
; CHECK-LT15:  %14 = getelementptr i8, i8 addrspace(1)* %2, i64 %11
; CHECK:  %15 = mul i64 %7, %4
; CHECK:  %16 = mul i64 %8, %4
; CHECK-GE15:  %17 = call spir_func ptr @__mux_dma_read_2D(ptr addrspace(3) %13, ptr addrspace(1) %14, i64 %12, i64 %16, i64 %15, i64 %6, ptr %9)
; CHECK-GE15:  ret ptr %17

; CHECK-LT15:  %17 = call spir_func %__mux_dma_event_t* @__mux_dma_read_2D(i8 addrspace(3)* %13, i8 addrspace(1)* %14, i64 %12, i64 %16, i64 %15, i64 %6, %__mux_dma_event_t* %mux.in.event)
; CHECK-LT15:  %clc.out.event = bitcast %__mux_dma_event_t* %17 to %opencl.event_t*
; CHECK-LT15:  ret %opencl.event_t* %clc.out.event
; CHECK: }

declare spir_func %opencl.event_t* @_Z26async_work_group_copy_3D3DPU3AS3vmPU3AS1Kvmmmmmmmmm9ocl_event(i8 addrspace(3)*, i64, i8 addrspace(1)*, i64, i64, i64, i64, i64, i64, i64, i64, i64, %opencl.event_t*)
; CHECK-LABEL-GE15: define spir_func ptr @_Z26async_work_group_copy_3D3DPU3AS3vmPU3AS1Kvmmmmmmmmm9ocl_event(
; CHECK-LABEL-LT15: define spir_func %opencl.event_t* @_Z26async_work_group_copy_3D3DPU3AS3vmPU3AS1Kvmmmmmmmmm9ocl_event(
; CHECK-LT15:  %mux.in.event = bitcast %opencl.event_t* %12 to %__mux_dma_event_t*
; CHECK:  %13 = mul i64 %1, %4
; CHECK:  %14 = mul i64 %3, %4
; CHECK:  %15 = mul i64 %5, %4
; CHECK-GE15:  %16 = getelementptr i8, ptr addrspace(3) %0, i64 %13
; CHECK-GE15:  %17 = getelementptr i8, ptr addrspace(1) %2, i64 %14
; CHECK-LT15:  %16 = getelementptr i8, i8 addrspace(3)* %0, i64 %13
; CHECK-LT15:  %17 = getelementptr i8, i8 addrspace(1)* %2, i64 %14
; CHECK:  %18 = mul i64 %8, %4
; CHECK:  %19 = mul i64 %10, %4
; CHECK:  %20 = mul i64 %9, %4
; CHECK:  %21 = mul i64 %11, %4
; CHECK-GE15:  %22 = call spir_func ptr @__mux_dma_read_3D(ptr addrspace(3) %16, ptr addrspace(1) %17, i64 %15, i64 %19, i64 %18, i64 %6, i64 %21, i64 %20, i64 %7, ptr %12)
; CHECK-GE15:  ret ptr %22

; CHECK-LT15:  %22 = call spir_func %__mux_dma_event_t* @__mux_dma_read_3D(i8 addrspace(3)* %16, i8 addrspace(1)* %17, i64 %15, i64 %19, i64 %18, i64 %6, i64 %21, i64 %20, i64 %7, %__mux_dma_event_t* %mux.in.event)
; CHECK-LT15:  %clc.out.event = bitcast %__mux_dma_event_t* %22 to %opencl.event_t*
; CHECK-LT15:  ret %opencl.event_t* %clc.out.event
; CHECK: }

declare spir_func void @_Z17wait_group_eventsiP9ocl_event(i32, %opencl.event_t**)
; CHECK-LABEL: define spir_func void @_Z17wait_group_eventsiP9ocl_event(
; CHECK-GE15:  call spir_func void @__mux_dma_wait(i32 %0, ptr %1)
; CHECK-LT15:  %mux.events = bitcast %opencl.event_t** %1 to %__mux_dma_event_t**
; CHECK-LT15:  call spir_func void @__mux_dma_wait(i32 %0, %__mux_dma_event_t** %mux.events)
; CHECK:  ret void
; CHECK: }
