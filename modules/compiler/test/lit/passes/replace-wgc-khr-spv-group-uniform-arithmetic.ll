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

; CHECK-DAG: @_Z29work_group_scan_inclusive_andj.accumulator = internal addrspace(3) global i32 undef
; CHECK-DAG: @_Z29work_group_scan_exclusive_andj.accumulator = internal addrspace(3) global i32 undef
; CHECK-DAG: @_Z37work_group_scan_inclusive_logical_andb.accumulator = internal addrspace(3) global i1 undef


; CHECK: define spir_func i32 @_Z29work_group_scan_inclusive_andj(i32 [[PARAM:%.*]])
declare spir_func i32 @_Z29work_group_scan_inclusive_andj(i32 %x)
; CHECK-LABEL: entry:
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_ONCE:#[0-9]+]]
; CHECK: store i32 -1, [[PTR_i32:.+]] @_Z29work_group_scan_inclusive_andj.accumulator
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_LINEAR:#[0-9]+]]
; CHECK: %[[CURRVAL:.+]] = load i32, [[PTR_i32]] @_Z29work_group_scan_inclusive_andj.accumulator
; CHECK: %[[SCAN:.+]] = call i32 @_Z28sub_group_scan_inclusive_andj(i32 %x)
; CHECK: %[[RESULT:.+]] = and i32 %[[CURRVAL]], %[[SCAN]]
; CHECK: %[[SIZE:.+]] = call i32 @_Z18get_sub_group_sizev()
; CHECK: %[[LAST:.+]] = sub nuw i32 %[[SIZE]], 1
; CHECK: %[[TAIL:.+]] = call i32 @_Z19sub_group_broadcastjj(i32 %[[SCAN]], i32 %[[LAST]])
; CHECK: %[[ACCUM:.+]] = and i32 %[[CURRVAL]], %[[TAIL]]
; CHECK: store i32 %[[ACCUM]], [[PTR_i32]] @_Z29work_group_scan_inclusive_andj.accumulator
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272)
; CHECK: ret i32 %[[RESULT]]


; CHECK: define spir_func i32 @_Z29work_group_scan_exclusive_andj(i32 [[PARAM:%.*]])
declare spir_func i32 @_Z29work_group_scan_exclusive_andj(i32 %x)
; CHECK-LABEL: entry:
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_ONCE]]
; CHECK: store i32 -1, [[PTR_i32]] @_Z29work_group_scan_exclusive_andj.accumulator
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_LINEAR]]
; CHECK: %[[CURRVAL:.+]] = load i32, [[PTR_i32]] @_Z29work_group_scan_exclusive_andj.accumulator
; CHECK: %[[SCAN:.+]] = call i32 @_Z28sub_group_scan_exclusive_andj(i32 %x)
; CHECK: %[[RESULT:.+]] = and i32 %[[CURRVAL]], %[[SCAN]]
; CHECK: %[[SIZE:.+]] = call i32 @_Z18get_sub_group_sizev()
; CHECK: %[[LAST:.+]] = sub nuw i32 %[[SIZE]], 1
; CHECK: %[[SCAN_TAIL:.+]] = call i32 @_Z19sub_group_broadcastjj(i32 %[[SCAN]], i32 %[[LAST]])
; CHECK: %[[TAIL:.+]] = call i32 @_Z19sub_group_broadcastjj(i32 [[PARAM]], i32 %[[LAST]])
; CHECK: %[[ACCUM0:.+]] = and i32 %[[SCAN_TAIL]], %[[TAIL]]
; CHECK: %[[ACCUM1:.+]] = and i32 %[[CURRVAL]], %[[ACCUM0]]
; CHECK: store i32 %[[ACCUM1]], [[PTR_i32]] @_Z29work_group_scan_exclusive_andj.accumulator
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272)
; CHECK: ret i32 %[[RESULT]]

; CHECK: define spir_func i1 @_Z37work_group_scan_inclusive_logical_andb(i1 [[PARAM:%.*]])
declare spir_func i1 @_Z37work_group_scan_inclusive_logical_andb(i1 %x)
; CHECK-LABEL: entry:
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_ONCE]]
; CHECK: store i1 true, [[PTR_i32]] @_Z37work_group_scan_inclusive_logical_andb.accumulator
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_LINEAR]]
; CHECK: %[[CURRVAL:.+]] = load i1, [[PTR_i32]] @_Z37work_group_scan_inclusive_logical_andb.accumulator
; CHECK: %[[SCAN:.+]] = call i1 @_Z36sub_group_scan_inclusive_logical_andb(i1 %x)
; CHECK: %[[RESULT:.+]] = and i1 %[[CURRVAL]], %[[SCAN]]
; CHECK: %[[SIZE:.+]] = call i32 @_Z18get_sub_group_sizev()
; CHECK: %[[LAST:.+]] = sub nuw i32 %[[SIZE]], 1
; CHECK: %[[TAIL:.+]] = call i1 @_Z19sub_group_broadcastbj(i1 %[[SCAN]], i32 %[[LAST]])
; CHECK: %[[ACCUM:.+]] = and i1 %[[CURRVAL]], %[[TAIL]]
; CHECK: store i1 %[[ACCUM]], [[PTR_i32]] @_Z37work_group_scan_inclusive_logical_andb.accumulator
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272)
; CHECK: ret i1 %[[RESULT]]


; CHECK-DAG: attributes [[SCHEDULE_ONCE]] = { "mux-barrier-schedule"="once" }
; CHECK-DAG: attributes [[SCHEDULE_LINEAR]] = { "mux-barrier-schedule"="linear" }

!opencl.ocl.version = !{!0}
!0 = !{i32 3, i32 0}
