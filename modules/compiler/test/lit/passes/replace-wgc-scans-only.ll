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

; RUN: muxc --passes "replace-wgc<scans-only>" -S %s | FileCheck %s

; Check that the replace-wgc only implements scans when run in scans-only mode

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: declare spir_func i32 @__mux_work_group_reduce_smin_i32({{.*}})
declare spir_func i32 @__mux_work_group_reduce_smin_i32(i32 %id, i32 %x)

; CHECK: define spir_func i32 @__mux_work_group_scan_inclusive_umax_i32({{.*}})
declare spir_func i32 @__mux_work_group_scan_inclusive_umax_i32(i32 %id, i32 %x)

; CHECK: define spir_func float @__mux_work_group_scan_exclusive_fadd_f32({{.*}})
declare spir_func float @__mux_work_group_scan_exclusive_fadd_f32(i32 %id, float %x)

; CHECK: declare spir_func i32 @__mux_work_group_broadcast_i32({{.*}})
declare spir_func i32 @__mux_work_group_broadcast_i32(i32 %barrier_id, i32 %x, i64 %idx, i64 %idy, i64 %idz)

; CHECK: define spir_func half @__mux_work_group_scan_exclusive_fadd_f16({{.*}})
declare spir_func half @__mux_work_group_scan_exclusive_fadd_f16(i32 %id, half %x)

!opencl.ocl.version = !{!0}
!0 = !{i32 3, i32 0}
