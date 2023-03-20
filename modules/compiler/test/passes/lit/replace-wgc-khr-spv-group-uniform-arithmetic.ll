; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes replace-wgc,verify -S %s -opaque-pointers | %filecheck %s

; Check that the replace-wgc correctly defines the work-group collective functions

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: @[[MULI_ACCUM:.+]] = internal addrspace(3) global i32 undef
; CHECK: @[[MULF_ACCUM:.+]] = internal addrspace(3) global float undef
; CHECK: @_Z29work_group_scan_inclusive_andj.accumulator = internal addrspace(3) global i32 undef
; CHECK: @_Z29work_group_scan_exclusive_andj.accumulator = internal addrspace(3) global i32 undef
; CHECK: @_Z38work_group_scan_inclusive_logical_andb.accumulator = internal addrspace(3) global i1 undef

; CHECK: define spir_func i32 @_Z21work_group_reduce_muli(i32 [[PARAM:%.*]])
declare spir_func i32 @_Z21work_group_reduce_muli(i32 %x)
; CHECK-LABEL: entry:
; CHECK: %[[SUBGROUP:.+]] = call i32 @_Z20sub_group_reduce_mulj(i32 %{{.+}})
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_ONCE:#[0-9]+]]
; CHECK: store i32 1, [[PTR_i32:(i32 addrspace\(3\)\*)|(ptr addrspace\(3\))]] @[[MULI_ACCUM]]
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272)
; CHECK: %[[CURRVAL:.+]] = load i32, [[PTR_i32]] @[[MULI_ACCUM]]
; CHECK: %[[ACCUM:.*]] = mul i32 %[[CURRVAL]], %[[SUBGROUP]]
; CHECK: store i32 %[[ACCUM]], [[PTR_i32]] @[[MULI_ACCUM]]
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272)
; CHECK: %[[RESULT:.*]] = load i32, [[PTR_i32]] @[[MULI_ACCUM]]
; CHECK: ret i32 %[[RESULT]]

; CHECK: define spir_func float @_Z21work_group_reduce_mulf(float [[PARAM:%.*]])
declare spir_func float @_Z21work_group_reduce_mulf(float %x)
; CHECK-LABEL: entry:
; CHECK: %[[SUBGROUP:.+]] = call float @_Z20sub_group_reduce_mulf(float %{{.+}})
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_ONCE:#[0-9]+]]
; CHECK: store float 1.000000e+00, [[PTR_f32:(float addrspace\(3\)\*)|(ptr addrspace\(3\))]] @[[MULF_ACCUM]]
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272)
; CHECK: %[[CURRVAL:.+]] = load float, [[PTR_f32]] @[[MULF_ACCUM]]
; CHECK: %[[ACCUM:.*]] = fmul float %[[CURRVAL]], %[[SUBGROUP]]
; CHECK: store float %[[ACCUM]], [[PTR_f32]] @[[MULF_ACCUM]]
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272)
; CHECK: %[[RESULT:.*]] = load float, [[PTR_i32]] @[[MULF_ACCUM]]
; CHECK: ret float %[[RESULT]]

; CHECK: define spir_func i32 @_Z29work_group_scan_inclusive_andj(i32 [[PARAM:%.*]])
declare spir_func i32 @_Z29work_group_scan_inclusive_andj(i32 %x)
; CHECK-LABEL: entry:
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_ONCE]]
; CHECK: store i32 -1, [[PTR_i32]] @_Z29work_group_scan_inclusive_andj.accumulator
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
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_LINEAR:#[0-9]+]]
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

; CHECK: define spir_func i1 @_Z38work_group_scan_inclusive_logical_andb(i1 [[PARAM:%.*]])
declare spir_func i1 @_Z38work_group_scan_inclusive_logical_andb(i1 %x)
; CHECK-LABEL: entry:
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_ONCE]]
; CHECK: store i1 true, [[PTR_i32]] @_Z38work_group_scan_inclusive_logical_andb.accumulator
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272) [[SCHEDULE_LINEAR:#[0-9]+]]
; CHECK: %[[CURRVAL:.+]] = load i1, [[PTR_i32]] @_Z38work_group_scan_inclusive_logical_andb.accumulator
; CHECK: %[[SCAN:.+]] = call i1 @_Z36sub_group_scan_inclusive_logical_andb(i1 %x)
; CHECK: %[[RESULT:.+]] = and i1 %[[CURRVAL]], %[[SCAN]]
; CHECK: %[[SIZE:.+]] = call i32 @_Z18get_sub_group_sizev()
; CHECK: %[[LAST:.+]] = sub nuw i32 %[[SIZE]], 1
; CHECK: %[[TAIL:.+]] = call i1 @_Z19sub_group_broadcastbj(i1 %[[SCAN]], i32 %[[LAST]])
; CHECK: %[[ACCUM:.+]] = and i1 %[[CURRVAL]], %[[TAIL]]
; CHECK: store i1 %[[ACCUM]], [[PTR_i32]] @_Z38work_group_scan_inclusive_logical_andb.accumulator
; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 272)
; CHECK: ret i1 %[[RESULT]]


; CHECK-DAG: attributes [[SCHEDULE_ONCE]] = { "mux-barrier-schedule"="once" }
; CHECK-DAG: attributes [[SCHEDULE_LINEAR]] = { "mux-barrier-schedule"="linear" }
