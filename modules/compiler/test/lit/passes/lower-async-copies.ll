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

; We also run early-cse as lower-to-mux-builtins produces redundant code that
; complicates the checks
; RUN: muxc --passes lower-to-mux-builtins,early-cse,verify -S %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @testfn(ptr %events, ptr addrspace(4) %events_generic,
                    ptr addrspace(3) %dst, ptr addrspace(1) %src, i64 %num_elts, i64 %stride) {
; CHECK: call void @__mux_dma_wait(i32 1, ptr %events)
  call spir_func void @_Z17wait_group_eventsiP9ocl_event(i32 1, ptr nonnull %events)
; CHECK: %mux.events = addrspacecast ptr addrspace(4) %events_generic to ptr
; CHECK: call void @__mux_dma_wait(i32 2, ptr %mux.events)
  call spir_func void @_Z17wait_group_eventsiPU3AS49ocl_event(i32 2, ptr addrspace(4) nonnull %events_generic)

; CHECK: [[BYTES:%.*]] = mul i64 4, %num_elts
; CHECK: %v1 = call ptr @__mux_dma_read_1D(ptr addrspace(3) %dst, ptr addrspace(1) %src, i64 [[BYTES]], ptr null)
  %v1 = call spir_func ptr @_Z21async_work_group_copyPU3AS3iPU3AS1Kim9ocl_event(ptr addrspace(3) %dst, ptr addrspace(1) %src, i64 %num_elts, ptr null)

; CHECK: %v2 = call ptr @__mux_dma_write_1D(ptr addrspace(1) %src, ptr addrspace(3) %dst, i64 [[BYTES]], ptr %events)
  %v2 = call spir_func ptr @_Z21async_work_group_copyPU3AS1iPU3AS3Kim9ocl_event(ptr addrspace(1) %src, ptr addrspace(3) %dst, i64 %num_elts, ptr %events)

; CHECK: [[STRIDE_BYTES:%.*]] = mul i64 4, %stride
; CHECK: %v3 = call ptr @__mux_dma_read_2D(ptr addrspace(3) %dst, ptr addrspace(1) %src, i64 4, i64 4, i64 [[STRIDE_BYTES]], i64 %num_elts, ptr null)
  %v3 = call spir_func ptr @_Z29async_work_group_strided_copyPU3AS3fPU3AS1Kfmm9ocl_event(ptr addrspace(3) %dst, ptr addrspace(1) %src, i64 %num_elts, i64 %stride, ptr null)

; CHECK: %v4 = call ptr @__mux_dma_write_2D(ptr addrspace(1) %src, ptr addrspace(3) %dst, i64 4, i64 [[STRIDE_BYTES]], i64 4, i64 %num_elts, ptr %events)
  %v4 = call spir_func ptr @_Z29async_work_group_strided_copyPU3AS1fPU3AS3Kfmm9ocl_event(ptr addrspace(1) %src, ptr addrspace(3) %dst, i64 %num_elts, i64 %stride, ptr %events)

  ret void
}

define void @testfn_2d2d(ptr addrspace(3) %dst, ptr addrspace(1) %src,
                         i64 %dst_offset, i64 %src_offset,
                         i64 %num_bytes_per_elt, i64 %num_elts_per_line, i64 %num_lines,
                         i64 %src_total_line_length, i64 %dst_total_line_length, ptr %event) {
; CHECK: %1 = mul i64 %dst_offset, %num_bytes_per_elt
; CHECK: %2 = mul i64 %src_offset, %num_bytes_per_elt
; CHECK: %3 = mul i64 %num_elts_per_line, %num_bytes_per_elt
; CHECK: %4 = getelementptr i8, ptr addrspace(3) %dst, i64 %1
; CHECK: %5 = getelementptr i8, ptr addrspace(1) %src, i64 %2
; CHECK: %6 = mul i64 %src_total_line_length, %num_bytes_per_elt
; CHECK: %7 = mul i64 %dst_total_line_length, %num_bytes_per_elt
; CHECK: %v0 = call ptr @__mux_dma_read_2D(ptr addrspace(3) %4, ptr addrspace(1) %5,
; CHECK-SAME:    i64 %3, i64 %7, i64 %6, i64 %num_lines, ptr %event)
  %v0 = call spir_func ptr @_Z26async_work_group_copy_2D2DPU3AS3vmPU3AS1Kvmmmmmm9ocl_event(
      ptr addrspace(3) %dst, i64 %dst_offset, ptr addrspace(1) %src, i64 %src_offset,
      i64 %num_bytes_per_elt, i64 %num_elts_per_line, i64 %num_lines,
      i64 %src_total_line_length, i64 %dst_total_line_length, ptr %event)

; CHECK: %8 = getelementptr i8, ptr addrspace(1) %src, i64 %1
; CHECK: %9 = getelementptr i8, ptr addrspace(3) %dst, i64 %2
; CHECK: %v1 = call ptr @__mux_dma_write_2D(ptr addrspace(1) %8, ptr addrspace(3) %9,
; CHECK-SAME:    i64 %3, i64 %7, i64 %6, i64 %num_lines, ptr %event)
  %v1 = call spir_func ptr @_Z26async_work_group_copy_2D2DPU3AS1vmPU3AS3Kvmmmmmm9ocl_event(
      ptr addrspace(1) %src, i64 %dst_offset, ptr addrspace(3) %dst, i64 %src_offset,
      i64 %num_bytes_per_elt, i64 %num_elts_per_line, i64 %num_lines,
      i64 %src_total_line_length, i64 %dst_total_line_length, ptr %event)
  ret void
}

define void @testfn_3d3d(ptr addrspace(3) %dst, ptr addrspace(1) %src,
                         i64 %dst_offset, i64 %src_offset,
                         i64 %num_bytes_per_elt, i64 %num_elts_per_line,
                         i64 %num_lines, i64 %num_planes,
                         i64 %src_total_line_length, i64 %dst_total_line_length,
                         i64 %src_total_plane_area, i64 %dst_total_plane_area, ptr %event) {
; CHECK: %1 = mul i64 %dst_offset, %num_bytes_per_elt
; CHECK: %2 = mul i64 %src_offset, %num_bytes_per_elt
; CHECK: %3 = mul i64 %num_elts_per_line, %num_bytes_per_elt
; CHECK: %4 = getelementptr i8, ptr addrspace(3) %dst, i64 %1
; CHECK: %5 = getelementptr i8, ptr addrspace(1) %src, i64 %2
; CHECK: %6 = mul i64 %src_total_line_length, %num_bytes_per_elt
; CHECK: %7 = mul i64 %dst_total_line_length, %num_bytes_per_elt
; CHECK: %8 = mul i64 %src_total_plane_area, %num_bytes_per_elt
; CHECK: %9 = mul i64 %dst_total_plane_area, %num_bytes_per_elt
; CHECK: %v0 = call ptr @__mux_dma_read_3D(ptr addrspace(3) %4, ptr addrspace(1) %5,
; CHECK-SAME:    i64 %3, i64 %7, i64 %6, i64 %num_lines, i64 %9, i64 %8, i64 %num_planes, ptr %event)
  %v0 = call spir_func ptr @_Z26async_work_group_copy_3D3DPU3AS3vmPU3AS1Kvmmmmmmmmm9ocl_event(
      ptr addrspace(3) %dst, i64 %dst_offset, ptr addrspace(1) %src, i64 %src_offset,
      i64 %num_bytes_per_elt, i64 %num_elts_per_line, i64 %num_lines, i64 %num_planes,
      i64 %src_total_line_length, i64 %src_total_plane_area,
      i64 %dst_total_line_length, i64 %dst_total_plane_area, ptr %event)
; CHECK: %10 = getelementptr i8, ptr addrspace(1) %src, i64 %1
; CHECK: %11 = getelementptr i8, ptr addrspace(3) %dst, i64 %2
; CHECK: %v1 = call ptr @__mux_dma_write_3D(ptr addrspace(1) %10, ptr addrspace(3) %11,
; CHECK-SAME:    i64 %3, i64 %7, i64 %6, i64 %num_lines, i64 %9, i64 %8, i64 %num_planes, ptr %event)
  %v1 = call spir_func ptr @_Z26async_work_group_copy_3D3DPU3AS1vmPU3AS3Kvmmmmmmmmm9ocl_event(
      ptr addrspace(1) %src, i64 %dst_offset, ptr addrspace(3) %dst, i64 %src_offset,
      i64 %num_bytes_per_elt, i64 %num_elts_per_line, i64 %num_lines, i64 %num_planes,
      i64 %src_total_line_length, i64 %src_total_plane_area,
      i64 %dst_total_line_length, i64 %dst_total_plane_area, ptr %event)
  ret void
}

declare spir_func ptr @_Z21async_work_group_copyPU3AS3iPU3AS1Kim9ocl_event(ptr addrspace(3), ptr addrspace(1), i64, ptr)

declare spir_func ptr @_Z21async_work_group_copyPU3AS1iPU3AS3Kim9ocl_event(ptr addrspace(1), ptr addrspace(3), i64, ptr)

declare spir_func ptr @_Z26async_work_group_copy_2D2DPU3AS3vmPU3AS1Kvmmmmmm9ocl_event(ptr addrspace(3), i64, ptr addrspace(1), i64, i64, i64, i64, i64, i64, ptr)
declare spir_func ptr @_Z26async_work_group_copy_2D2DPU3AS1vmPU3AS3Kvmmmmmm9ocl_event(ptr addrspace(1), i64, ptr addrspace(3), i64, i64, i64, i64, i64, i64, ptr)

declare spir_func ptr @_Z26async_work_group_copy_3D3DPU3AS3vmPU3AS1Kvmmmmmmmmm9ocl_event(ptr addrspace(3), i64, ptr addrspace(1), i64, i64, i64, i64, i64, i64, i64, i64, i64, ptr)
declare spir_func ptr @_Z26async_work_group_copy_3D3DPU3AS1vmPU3AS3Kvmmmmmmmmm9ocl_event(ptr addrspace(1), i64, ptr addrspace(3), i64, i64, i64, i64, i64, i64, i64, i64, i64, ptr)

declare spir_func void @_Z17wait_group_eventsiP9ocl_event(i32, ptr)
declare spir_func void @_Z17wait_group_eventsiPU3AS49ocl_event(i32, ptr addrspace(4))

declare spir_func ptr @_Z29async_work_group_strided_copyPU3AS3fPU3AS1Kfmm9ocl_event(ptr addrspace(3), ptr addrspace(1), i64, i64, ptr)
declare spir_func ptr @_Z29async_work_group_strided_copyPU3AS1fPU3AS3Kfmm9ocl_event(ptr addrspace(1), ptr addrspace(3), i64, i64, ptr)
