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

; RUN: muxc --passes replace-wgc,verify -S %s  | FileCheck %s

; Check that the replace-wgc correctly defines the work-group collective functions

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK-DAG: @[[MULI_ACCUM:.+]] = internal addrspace(3) global i32 undef
; CHECK-DAG: @[[MULF_ACCUM:.+]] = internal addrspace(3) global float undef
; CHECK-DAG: @[[MULH_ACCUM:.+]] = internal addrspace(3) global half undef
; CHECK-DAG: @__mux_work_group_scan_inclusive_and_i32.accumulator = internal addrspace(3) global i32 undef
; CHECK-DAG: @__mux_work_group_scan_exclusive_and_i32.accumulator = internal addrspace(3) global i32 undef
; CHECK-DAG: @__mux_work_group_scan_inclusive_logical_and_i1.accumulator = internal addrspace(3) global i1 undef

; CHECK: define spir_func i32 @__mux_work_group_reduce_mul_i32(i32 %id, i32 [[PARAM:%.*]])
declare spir_func i32 @__mux_work_group_reduce_mul_i32(i32 %id, i32 %x)
; CHECK-LABEL: entry:
; CHECK: %[[SUBGROUP:.+]] = call i32 @__mux_sub_group_reduce_mul_i32(i32 %{{.+}})
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_ONCE:#[0-9]+]]
; CHECK: store i32 1, [[PTR_i32:(i32 addrspace\(3\)\*)|(ptr addrspace\(3\))]] @[[MULI_ACCUM]]
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272)
; CHECK: %[[CURRVAL:.+]] = load i32, [[PTR_i32]] @[[MULI_ACCUM]]
; CHECK: %[[ACCUM:.*]] = mul i32 %[[CURRVAL]], %[[SUBGROUP]]
; CHECK: store i32 %[[ACCUM]], [[PTR_i32]] @[[MULI_ACCUM]]
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272)
; CHECK: %[[RESULT:.*]] = load i32, [[PTR_i32]] @[[MULI_ACCUM]]
; CHECK: ret i32 %[[RESULT]]

; CHECK: define spir_func float @__mux_work_group_reduce_fmul_f32(i32 %id, float [[PARAM:%.*]])
declare spir_func float @__mux_work_group_reduce_fmul_f32(i32 %id, float %x)
; CHECK-LABEL: entry:
; CHECK: %[[SUBGROUP:.+]] = call float @__mux_sub_group_reduce_fmul_f32(float %{{.+}})
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_ONCE:#[0-9]+]]
; CHECK: store float 1.000000e+00, [[PTR_f32:(float addrspace\(3\)\*)|(ptr addrspace\(3\))]] @[[MULF_ACCUM]]
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272)
; CHECK: %[[CURRVAL:.+]] = load float, [[PTR_f32]] @[[MULF_ACCUM]]
; CHECK: %[[ACCUM:.*]] = fmul float %[[CURRVAL]], %[[SUBGROUP]]
; CHECK: store float %[[ACCUM]], [[PTR_f32]] @[[MULF_ACCUM]]
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272)
; CHECK: %[[RESULT:.*]] = load float, [[PTR_i32]] @[[MULF_ACCUM]]
; CHECK: ret float %[[RESULT]]

; CHECK: define spir_func i32 @__mux_work_group_scan_inclusive_and_i32(i32 %id, i32 [[PARAM:%.*]])
declare spir_func i32 @__mux_work_group_scan_inclusive_and_i32(i32 %id, i32 %x)
; CHECK-LABEL: entry:
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_ONCE]]
; CHECK: store i32 -1, [[PTR_i32]] @__mux_work_group_scan_inclusive_and_i32.accumulator
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_LINEAR:#[0-9]+]]
; CHECK: %[[CURRVAL:.+]] = load i32, [[PTR_i32]] @__mux_work_group_scan_inclusive_and_i32.accumulator
; CHECK: %[[SCAN:.+]] = call i32 @__mux_sub_group_scan_inclusive_and_i32(i32 %x)
; CHECK: %[[RESULT:.+]] = and i32 %[[CURRVAL]], %[[SCAN]]
; CHECK: %[[SIZE:.+]] = call i32 @__mux_get_sub_group_size()
; CHECK: %[[LAST:.+]] = sub nuw i32 %[[SIZE]], 1
; CHECK: %[[TAIL:.+]] = call i32 @__mux_sub_group_broadcast_i32(i32 %[[SCAN]], i32 %[[LAST]])
; CHECK: %[[ACCUM:.+]] = and i32 %[[CURRVAL]], %[[TAIL]]
; CHECK: store i32 %[[ACCUM]], [[PTR_i32]] @__mux_work_group_scan_inclusive_and_i32.accumulator
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272)
; CHECK: ret i32 %[[RESULT]]


; CHECK: define spir_func i32 @__mux_work_group_scan_exclusive_and_i32(i32 %id, i32 [[PARAM:%.*]])
declare spir_func i32 @__mux_work_group_scan_exclusive_and_i32(i32 %id, i32 %x)
; CHECK-LABEL: entry:
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_ONCE]]
; CHECK: store i32 -1, [[PTR_i32]] @__mux_work_group_scan_exclusive_and_i32.accumulator
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_LINEAR:#[0-9]+]]
; CHECK: %[[CURRVAL:.+]] = load i32, [[PTR_i32]] @__mux_work_group_scan_exclusive_and_i32.accumulator
; CHECK: %[[SCAN:.+]] = call i32 @__mux_sub_group_scan_exclusive_and_i32(i32 %x)
; CHECK: %[[RESULT:.+]] = and i32 %[[CURRVAL]], %[[SCAN]]
; CHECK: %[[SIZE:.+]] = call i32 @__mux_get_sub_group_size()
; CHECK: %[[LAST:.+]] = sub nuw i32 %[[SIZE]], 1
; CHECK: %[[SCAN_TAIL:.+]] = call i32 @__mux_sub_group_broadcast_i32(i32 %[[SCAN]], i32 %[[LAST]])
; CHECK: %[[TAIL:.+]] = call i32 @__mux_sub_group_broadcast_i32(i32 [[PARAM]], i32 %[[LAST]])
; CHECK: %[[ACCUM0:.+]] = and i32 %[[SCAN_TAIL]], %[[TAIL]]
; CHECK: %[[ACCUM1:.+]] = and i32 %[[CURRVAL]], %[[ACCUM0]]
; CHECK: store i32 %[[ACCUM1]], [[PTR_i32]] @__mux_work_group_scan_exclusive_and_i32.accumulator
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272)
; CHECK: ret i32 %[[RESULT]]

; CHECK: define spir_func i1 @__mux_work_group_scan_inclusive_logical_and_i1(i32 %id, i1 [[PARAM:%.*]])
declare spir_func i1 @__mux_work_group_scan_inclusive_logical_and_i1(i32 %id, i1 %x)
; CHECK-LABEL: entry:
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_ONCE]]
; CHECK: store i1 true, [[PTR_i32]] @__mux_work_group_scan_inclusive_logical_and_i1.accumulator
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_LINEAR:#[0-9]+]]
; CHECK: %[[CURRVAL:.+]] = load i1, [[PTR_i32]] @__mux_work_group_scan_inclusive_logical_and_i1.accumulator
; CHECK: %[[SCAN:.+]] = call i1 @__mux_sub_group_scan_inclusive_logical_and_i1(i1 %x)
; CHECK: %[[RESULT:.+]] = and i1 %[[CURRVAL]], %[[SCAN]]
; CHECK: %[[SIZE:.+]] = call i32 @__mux_get_sub_group_size()
; CHECK: %[[LAST:.+]] = sub nuw i32 %[[SIZE]], 1
; CHECK: %[[TAIL:.+]] = call i1 @__mux_sub_group_broadcast_i1(i1 %[[SCAN]], i32 %[[LAST]])
; CHECK: %[[ACCUM:.+]] = and i1 %[[CURRVAL]], %[[TAIL]]
; CHECK: store i1 %[[ACCUM]], [[PTR_i32]] @__mux_work_group_scan_inclusive_logical_and_i1.accumulator
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272)
; CHECK: ret i1 %[[RESULT]]

; CHECK: define spir_func half @__mux_work_group_reduce_fmul_f16(i32 %id, half [[PARAM:%.*]])
declare spir_func half @__mux_work_group_reduce_fmul_f16(i32 %id, half %x)
; CHECK-LABEL: entry:
; CHECK: %[[SUBGROUP:.+]] = call half @__mux_sub_group_reduce_fmul_f16(half %{{.+}})
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_ONCE:#[0-9]+]]
; CHECK: store half 0xH3C00, [[PTR_f16:(half addrspace\(3\)\*)|(ptr addrspace\(3\))]] @[[MULH_ACCUM]]
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272)
; CHECK: %[[CURRVAL:.+]] = load half, [[PTR_f16]] @[[MULH_ACCUM]]
; CHECK: %[[ACCUM:.*]] = fmul half %[[CURRVAL]], %[[SUBGROUP]]
; CHECK: store half %[[ACCUM]], [[PTR_f16]] @[[MULH_ACCUM]]
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272)
; CHECK: %[[RESULT:.*]] = load half, [[PTR_i32]] @[[MULH_ACCUM]]
; CHECK: ret half %[[RESULT]]


; CHECK-DAG: attributes [[SCHEDULE_ONCE]] = { "mux-barrier-schedule"="once" }
; CHECK-DAG: attributes [[SCHEDULE_LINEAR]] = { "mux-barrier-schedule"="linear" }

!opencl.ocl.version = !{!0}
!0 = !{i32 3, i32 0}
