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
; RUN: muxc --passes lower-to-mux-builtins,verify %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

define spir_func i32 @sub_group_size_test() {
; CHECK: %call = call i32 @__mux_get_sub_group_size()
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z18get_sub_group_sizev()
  ret i32 %call
}

define spir_func i32 @sub_group_local_id_test() {
; CHECK: %call = call i32 @__mux_get_sub_group_local_id()
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z22get_sub_group_local_idv()
  ret i32 %call
}

define spir_func i32 @sub_group_all_test(i32 %x) {
; CHECK: [[T0:%.*]] = icmp ne i32 %x, 0
; CHECK: %call = call i1 @__mux_sub_group_all_i1(i1 [[T0]])
; CHECK: [[T1:%.*]] = sext i1 %call to i32
; CHECK: ret i32 [[T1]]
  %call = call spir_func i32 @_Z13sub_group_alli(i32 %x)
  ret i32 %call
}

define spir_func i32 @sub_group_any_test(i32 %x) {
; CHECK: [[T0:%.*]] = icmp ne i32 %x, 0
; CHECK: %call = call i1 @__mux_sub_group_any_i1(i1 [[T0]])
; CHECK: [[T1:%.*]] = sext i1 %call to i32
; CHECK: ret i32 [[T1]]
  %call = call spir_func i32 @_Z13sub_group_anyi(i32 %x)
  ret i32 %call
}

define spir_func i32 @sub_group_broadcasti_test(i32 %x, i32 %lid) {
; CHECK: %call = call i32 @__mux_sub_group_broadcast_i32(i32 %x, i32 %lid)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z19sub_group_broadcastij(i32 %x, i32 %lid)
  ret i32 %call
}

define spir_func float @sub_group_broadcastf_test(float %x, i32 %lid) {
; CHECK: %call = call float @__mux_sub_group_broadcast_f32(float %x, i32 %lid)
; CHECK: ret float %call
  %call = call spir_func float @_Z19sub_group_broadcastfj(float %x, i32 %lid)
  ret float %call
}

define spir_func i32 @sub_group_reduce_addi_test(i32 %x) {
; CHECK: %call = call i32 @__mux_sub_group_reduce_add_i32(i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z20sub_group_reduce_addi(i32 %x)
  ret i32 %call
}

define spir_func float @sub_group_reduce_addf_test(float %x) {
; CHECK: %call = call float @__mux_sub_group_reduce_fadd_f32(float %x)
; CHECK: ret float %call
  %call = call spir_func float @_Z20sub_group_reduce_addf(float %x)
  ret float %call
}

define spir_func i32 @sub_group_reduce_mini_test(i32 %x) {
; CHECK: %call = call i32 @__mux_sub_group_reduce_smin_i32(i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z20sub_group_reduce_mini(i32 %x)
  ret i32 %call
}

define spir_func i32 @sub_group_reduce_minu_test(i32 %x) {
; CHECK: %call = call i32 @__mux_sub_group_reduce_umin_i32(i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z20sub_group_reduce_minj(i32 %x)
  ret i32 %call
}

define spir_func float @sub_group_reduce_minf_test(float %x) {
; CHECK: %call = call float @__mux_sub_group_reduce_fmin_f32(float %x)
; CHECK: ret float %call
  %call = call spir_func float @_Z20sub_group_reduce_minf(float %x)
  ret float %call
}

define spir_func i32 @sub_group_reduce_maxi_test(i32 %x) {
; CHECK: %call = call i32 @__mux_sub_group_reduce_smax_i32(i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z20sub_group_reduce_maxi(i32 %x)
  ret i32 %call
}

define spir_func i32 @sub_group_reduce_maxj_test(i32 %x) {
; CHECK: %call = call i32 @__mux_sub_group_reduce_umax_i32(i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z20sub_group_reduce_maxj(i32 %x)
  ret i32 %call
}

define spir_func float @sub_group_reduce_maxf_test(float %x) {
; CHECK: %call = call float @__mux_sub_group_reduce_fmax_f32(float %x)
; CHECK: ret float %call
  %call = call spir_func float @_Z20sub_group_reduce_maxf(float %x)
  ret float %call
}

define spir_func i32 @sub_group_scan_exclusive_addi_test(i32 %x) {
; CHECK: %call = call i32 @__mux_sub_group_scan_exclusive_add_i32(i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z28sub_group_scan_exclusive_addi(i32 %x)
  ret i32 %call
}

define spir_func i32 @sub_group_scan_exclusive_addj_test(i32 %x) {
; CHECK: %call = call i32 @__mux_sub_group_scan_exclusive_add_i32(i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z28sub_group_scan_exclusive_addj(i32 %x)
  ret i32 %call
}

define spir_func float @sub_group_scan_exclusive_addf_test(float %x) {
; CHECK: %call = call float @__mux_sub_group_scan_exclusive_fadd_f32(float %x)
; CHECK: ret float %call
  %call = call spir_func float @_Z28sub_group_scan_exclusive_addf(float %x)
  ret float %call
}

define spir_func i32 @sub_group_scan_exclusive_mini_test(i32 %x) {
; CHECK: %call = call i32 @__mux_sub_group_scan_exclusive_smin_i32(i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z28sub_group_scan_exclusive_mini(i32 %x)
  ret i32 %call
}

define spir_func i32 @sub_group_scan_exclusive_minj_test(i32 %x) {
; CHECK: %call = call i32 @__mux_sub_group_scan_exclusive_umin_i32(i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z28sub_group_scan_exclusive_minj(i32 %x)
  ret i32 %call
}

define spir_func float @sub_group_scan_exclusive_minf_test(float %x) {
; CHECK: %call = call float @__mux_sub_group_scan_exclusive_fmin_f32(float %x)
; CHECK: ret float %call
  %call = call spir_func float @_Z28sub_group_scan_exclusive_minf(float %x)
  ret float %call
}

define spir_func i32 @sub_group_scan_exclusive_maxi_test(i32 %x) {
; CHECK: %call = call i32 @__mux_sub_group_scan_exclusive_smax_i32(i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z28sub_group_scan_exclusive_maxi(i32 %x)
  ret i32 %call
}

define spir_func i32 @sub_group_scan_exclusive_maxj_test(i32 %x) {
; CHECK: %call = call i32 @__mux_sub_group_scan_exclusive_umax_i32(i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z28sub_group_scan_exclusive_maxj(i32 %x)
  ret i32 %call
}

define spir_func float @sub_group_scan_exclusive_maxf_test(float %x) {
; CHECK: %call = call float @__mux_sub_group_scan_exclusive_fmax_f32(float %x)
; CHECK: ret float %call
  %call = call spir_func float @_Z28sub_group_scan_exclusive_maxf(float %x)
  ret float %call
}

define spir_func i32 @sub_group_scan_inclusive_addi_test(i32 %x) {
; CHECK: %call = call i32 @__mux_sub_group_scan_inclusive_add_i32(i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z28sub_group_scan_inclusive_addi(i32 %x)
  ret i32 %call
}

define spir_func i32 @sub_group_scan_inclusive_addj_test(i32 %x) {
; CHECK: %call = call i32 @__mux_sub_group_scan_inclusive_add_i32(i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z28sub_group_scan_inclusive_addj(i32 %x)
  ret i32 %call
}

define spir_func float @sub_group_scan_inclusive_addf_test(float %x) {
; CHECK: %call = call float @__mux_sub_group_scan_inclusive_fadd_f32(float %x)
; CHECK: ret float %call
  %call = call spir_func float @_Z28sub_group_scan_inclusive_addf(float %x)
  ret float %call
}

define spir_func i32 @sub_group_scan_inclusive_mini_test(i32 %x) {
; CHECK: %call = call i32 @__mux_sub_group_scan_inclusive_smin_i32(i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z28sub_group_scan_inclusive_mini(i32 %x)
  ret i32 %call
}

define spir_func i32 @sub_group_scan_inclusive_minj_test(i32 %x) {
; CHECK: %call = call i32 @__mux_sub_group_scan_inclusive_umin_i32(i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z28sub_group_scan_inclusive_minj(i32 %x)
  ret i32 %call
}

define spir_func float @sub_group_scan_inclusive_minf_test(float %x) {
; CHECK: %call = call float @__mux_sub_group_scan_inclusive_fmin_f32(float %x)
; CHECK: ret float %call
  %call = call spir_func float @_Z28sub_group_scan_inclusive_minf(float %x)
  ret float %call
}

define spir_func i32 @sub_group_scan_inclusive_maxi_test(i32 %x) {
; CHECK: %call = call i32 @__mux_sub_group_scan_inclusive_smax_i32(i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z28sub_group_scan_inclusive_maxi(i32 %x)
  ret i32 %call
}

define spir_func i32 @sub_group_scan_inclusive_maxj_test(i32 %x) {
; CHECK: %call = call i32 @__mux_sub_group_scan_inclusive_umax_i32(i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z28sub_group_scan_inclusive_maxj(i32 %x)
  ret i32 %call
}

define spir_func float @sub_group_scan_inclusive_maxf_test(float %x) {
; CHECK: %call = call float @__mux_sub_group_scan_inclusive_fmax_f32(float %x)
; CHECK: ret float %call
  %call = call spir_func float @_Z28sub_group_scan_inclusive_maxf(float %x)
  ret float %call
}

declare spir_func i32 @_Z18get_sub_group_sizev()
declare spir_func i32 @_Z22get_sub_group_local_idv()
declare spir_func i32 @_Z13sub_group_alli(i32)
declare spir_func i32 @_Z13sub_group_anyi(i32)
declare spir_func i32 @_Z19sub_group_broadcastij(i32, i32)
declare spir_func float @_Z19sub_group_broadcastfj(float, i32)
declare spir_func i32 @_Z20sub_group_reduce_addi(i32)
declare spir_func float @_Z20sub_group_reduce_addf(float)
declare spir_func i32 @_Z20sub_group_reduce_mini(i32)
declare spir_func i32 @_Z20sub_group_reduce_minj(i32)
declare spir_func float @_Z20sub_group_reduce_minf(float)
declare spir_func i32 @_Z20sub_group_reduce_maxi(i32)
declare spir_func i32 @_Z20sub_group_reduce_maxj(i32)
declare spir_func float @_Z20sub_group_reduce_maxf(float)
declare spir_func i32 @_Z28sub_group_scan_exclusive_addi(i32)
declare spir_func i32 @_Z28sub_group_scan_exclusive_addj(i32)
declare spir_func float @_Z28sub_group_scan_exclusive_addf(float)
declare spir_func i32 @_Z28sub_group_scan_exclusive_mini(i32)
declare spir_func i32 @_Z28sub_group_scan_exclusive_minj(i32)
declare spir_func float @_Z28sub_group_scan_exclusive_minf(float)
declare spir_func i32 @_Z28sub_group_scan_exclusive_maxi(i32)
declare spir_func i32 @_Z28sub_group_scan_exclusive_maxj(i32)
declare spir_func float @_Z28sub_group_scan_exclusive_maxf(float)
declare spir_func i32 @_Z28sub_group_scan_inclusive_addi(i32)
declare spir_func i32 @_Z28sub_group_scan_inclusive_addj(i32)
declare spir_func float @_Z28sub_group_scan_inclusive_addf(float)
declare spir_func i32 @_Z28sub_group_scan_inclusive_mini(i32)
declare spir_func i32 @_Z28sub_group_scan_inclusive_minj(i32)
declare spir_func float @_Z28sub_group_scan_inclusive_minf(float)
declare spir_func i32 @_Z28sub_group_scan_inclusive_maxi(i32)
declare spir_func i32 @_Z28sub_group_scan_inclusive_maxj(i32)
declare spir_func float @_Z28sub_group_scan_inclusive_maxf(float)

!opencl.ocl.version = !{!0}

!0 = !{i32 3, i32 0}
