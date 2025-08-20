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
; RUN: muxc --passes lower-to-mux-builtins,verify %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

define spir_func i32 @work_group_all_test(i32 %x) {
; CHECK: [[T0:%.*]] = icmp ne i32 %x, 0
; CHECK: %call = call i1 @__mux_work_group_all_i1(i32 0, i1 [[T0]])
; CHECK: [[T1:%.*]] = sext i1 %call to i32
; CHECK: ret i32 [[T1]]
  %call = call spir_func i32 @_Z14work_group_alli(i32 %x)
  ret i32 %call
}

define spir_func i32 @work_group_any_test(i32 %x) {
; CHECK: [[T0:%.*]] = icmp ne i32 %x, 0
; CHECK: %call = call i1 @__mux_work_group_any_i1(i32 0, i1 [[T0]])
; CHECK: [[T1:%.*]] = sext i1 %call to i32
; CHECK: ret i32 [[T1]]
  %call = call spir_func i32 @_Z14work_group_anyi(i32 %x)
  ret i32 %call
}

define spir_func i32 @work_group_broadcastxi_test(i32 %x, i64 %lid) {
; CHECK: %call = call i32 @__mux_work_group_broadcast_i32(i32 0, i32 %x, i64 %lid, i64 0, i64 0)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z20work_group_broadcastim(i32 %x, i64 %lid)
  ret i32 %call
}

define spir_func float @work_group_broadcastxf_test(float %x, i64 %lid) {
; CHECK: %call = call float @__mux_work_group_broadcast_f32(i32 0, float %x, i64 %lid, i64 0, i64 0)
; CHECK: ret float %call
  %call = call spir_func float @_Z20work_group_broadcastfm(float %x, i64 %lid)
  ret float %call
}

define spir_func i32 @work_group_broadcastxyi_test(i32 %x, i64 %lidx, i64 %lidy) {
; CHECK: %call = call i32 @__mux_work_group_broadcast_i32(i32 0, i32 %x, i64 %lidx, i64 %lidy, i64 0)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z20work_group_broadcastimm(i32 %x, i64 %lidx, i64 %lidy)
  ret i32 %call
}

define spir_func float @work_group_broadcastxyf_test(float %x, i64 %lidx, i64 %lidy) {
; CHECK: %call = call float @__mux_work_group_broadcast_f32(i32 0, float %x, i64 %lidx, i64 %lidy, i64 0)
; CHECK: ret float %call
  %call = call spir_func float @_Z20work_group_broadcastfmm(float %x, i64 %lidx, i64 %lidy)
  ret float %call
}

define spir_func i32 @work_group_broadcastxyzi_test(i32 %x, i64 %lidx, i64 %lidy, i64 %lidz) {
; CHECK: %call = call i32 @__mux_work_group_broadcast_i32(i32 0, i32 %x, i64 %lidx, i64 %lidy, i64 %lidz)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z20work_group_broadcastimmm(i32 %x, i64 %lidx, i64 %lidy, i64 %lidz)
  ret i32 %call
}

define spir_func float @work_group_broadcastxyzf_test(float %x, i64 %lidx, i64 %lidy, i64 %lidz) {
; CHECK: %call = call float @__mux_work_group_broadcast_f32(i32 0, float %x, i64 %lidx, i64 %lidy, i64 %lidz)
; CHECK: ret float %call
  %call = call spir_func float @_Z20work_group_broadcastfmmm(float %x, i64 %lidx, i64 %lidy, i64 %lidz)
  ret float %call
}

define spir_func i32 @work_group_reduce_addi_test(i32 %x) {
; CHECK: %call = call i32 @__mux_work_group_reduce_add_i32(i32 0, i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z21work_group_reduce_addi(i32 %x)
  ret i32 %call
}

define spir_func float @work_group_reduce_addf_test(float %x) {
; CHECK: %call = call float @__mux_work_group_reduce_fadd_f32(i32 0, float %x)
; CHECK: ret float %call
  %call = call spir_func float @_Z21work_group_reduce_addf(float %x)
  ret float %call
}

define spir_func i32 @work_group_reduce_mini_test(i32 %x) {
; CHECK: %call = call i32 @__mux_work_group_reduce_smin_i32(i32 0, i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z21work_group_reduce_mini(i32 %x)
  ret i32 %call
}

define spir_func i32 @work_group_reduce_minu_test(i32 %x) {
; CHECK: %call = call i32 @__mux_work_group_reduce_umin_i32(i32 0, i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z21work_group_reduce_minj(i32 %x)
  ret i32 %call
}

define spir_func float @work_group_reduce_minf_test(float %x) {
; CHECK: %call = call float @__mux_work_group_reduce_fmin_f32(i32 0, float %x)
; CHECK: ret float %call
  %call = call spir_func float @_Z21work_group_reduce_minf(float %x)
  ret float %call
}

define spir_func i32 @work_group_reduce_maxi_test(i32 %x) {
; CHECK: %call = call i32 @__mux_work_group_reduce_smax_i32(i32 0, i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z21work_group_reduce_maxi(i32 %x)
  ret i32 %call
}

define spir_func i32 @work_group_reduce_maxj_test(i32 %x) {
; CHECK: %call = call i32 @__mux_work_group_reduce_umax_i32(i32 0, i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z21work_group_reduce_maxj(i32 %x)
  ret i32 %call
}

define spir_func float @work_group_reduce_maxf_test(float %x) {
; CHECK: %call = call float @__mux_work_group_reduce_fmax_f32(i32 0, float %x)
; CHECK: ret float %call
  %call = call spir_func float @_Z21work_group_reduce_maxf(float %x)
  ret float %call
}

define spir_func i32 @work_group_scan_exclusive_addi_test(i32 %x) {
; CHECK: %call = call i32 @__mux_work_group_scan_exclusive_add_i32(i32 0, i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z29work_group_scan_exclusive_addi(i32 %x)
  ret i32 %call
}

define spir_func i32 @work_group_scan_exclusive_addj_test(i32 %x) {
; CHECK: %call = call i32 @__mux_work_group_scan_exclusive_add_i32(i32 0, i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z29work_group_scan_exclusive_addj(i32 %x)
  ret i32 %call
}

define spir_func float @work_group_scan_exclusive_addf_test(float %x) {
; CHECK: %call = call float @__mux_work_group_scan_exclusive_fadd_f32(i32 0, float %x)
; CHECK: ret float %call
  %call = call spir_func float @_Z29work_group_scan_exclusive_addf(float %x)
  ret float %call
}

define spir_func i32 @work_group_scan_exclusive_mini_test(i32 %x) {
; CHECK: %call = call i32 @__mux_work_group_scan_exclusive_smin_i32(i32 0, i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z29work_group_scan_exclusive_mini(i32 %x)
  ret i32 %call
}

define spir_func i32 @work_group_scan_exclusive_minj_test(i32 %x) {
; CHECK: %call = call i32 @__mux_work_group_scan_exclusive_umin_i32(i32 0, i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z29work_group_scan_exclusive_minj(i32 %x)
  ret i32 %call
}

define spir_func float @work_group_scan_exclusive_minf_test(float %x) {
; CHECK: %call = call float @__mux_work_group_scan_exclusive_fmin_f32(i32 0, float %x)
; CHECK: ret float %call
  %call = call spir_func float @_Z29work_group_scan_exclusive_minf(float %x)
  ret float %call
}

define spir_func i32 @work_group_scan_exclusive_maxi_test(i32 %x) {
; CHECK: %call = call i32 @__mux_work_group_scan_exclusive_smax_i32(i32 0, i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z29work_group_scan_exclusive_maxi(i32 %x)
  ret i32 %call
}

define spir_func i32 @work_group_scan_exclusive_maxj_test(i32 %x) {
; CHECK: %call = call i32 @__mux_work_group_scan_exclusive_umax_i32(i32 0, i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z29work_group_scan_exclusive_maxj(i32 %x)
  ret i32 %call
}

define spir_func float @work_group_scan_exclusive_maxf_test(float %x) {
; CHECK: %call = call float @__mux_work_group_scan_exclusive_fmax_f32(i32 0, float %x)
; CHECK: ret float %call
  %call = call spir_func float @_Z29work_group_scan_exclusive_maxf(float %x)
  ret float %call
}

define spir_func i32 @work_group_scan_inclusive_addi_test(i32 %x) {
; CHECK: %call = call i32 @__mux_work_group_scan_inclusive_add_i32(i32 0, i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z29work_group_scan_inclusive_addi(i32 %x)
  ret i32 %call
}

define spir_func i32 @work_group_scan_inclusive_addj_test(i32 %x) {
; CHECK: %call = call i32 @__mux_work_group_scan_inclusive_add_i32(i32 0, i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z29work_group_scan_inclusive_addj(i32 %x)
  ret i32 %call
}

define spir_func float @work_group_scan_inclusive_addf_test(float %x) {
; CHECK: %call = call float @__mux_work_group_scan_inclusive_fadd_f32(i32 0, float %x)
; CHECK: ret float %call
  %call = call spir_func float @_Z29work_group_scan_inclusive_addf(float %x)
  ret float %call
}

define spir_func i32 @work_group_scan_inclusive_mini_test(i32 %x) {
; CHECK: %call = call i32 @__mux_work_group_scan_inclusive_smin_i32(i32 0, i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z29work_group_scan_inclusive_mini(i32 %x)
  ret i32 %call
}

define spir_func i32 @work_group_scan_inclusive_minj_test(i32 %x) {
; CHECK: %call = call i32 @__mux_work_group_scan_inclusive_umin_i32(i32 0, i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z29work_group_scan_inclusive_minj(i32 %x)
  ret i32 %call
}

define spir_func float @work_group_scan_inclusive_minf_test(float %x) {
; CHECK: %call = call float @__mux_work_group_scan_inclusive_fmin_f32(i32 0, float %x)
; CHECK: ret float %call
  %call = call spir_func float @_Z29work_group_scan_inclusive_minf(float %x)
  ret float %call
}

define spir_func i32 @work_group_scan_inclusive_maxi_test(i32 %x) {
; CHECK: %call = call i32 @__mux_work_group_scan_inclusive_smax_i32(i32 0, i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z29work_group_scan_inclusive_maxi(i32 %x)
  ret i32 %call
}

define spir_func i32 @work_group_scan_inclusive_maxj_test(i32 %x) {
; CHECK: %call = call i32 @__mux_work_group_scan_inclusive_umax_i32(i32 0, i32 %x)
; CHECK: ret i32 %call
  %call = call spir_func i32 @_Z29work_group_scan_inclusive_maxj(i32 %x)
  ret i32 %call
}

define spir_func float @work_group_scan_inclusive_maxf_test(float %x) {
; CHECK: %call = call float @__mux_work_group_scan_inclusive_fmax_f32(i32 0, float %x)
; CHECK: ret float %call
  %call = call spir_func float @_Z29work_group_scan_inclusive_maxf(float %x)
  ret float %call
}

declare spir_func i32 @_Z14work_group_alli(i32)
declare spir_func i32 @_Z14work_group_anyi(i32)
declare spir_func i32 @_Z20work_group_broadcastim(i32, i64)
declare spir_func float @_Z20work_group_broadcastfm(float, i64)
declare spir_func i32 @_Z20work_group_broadcastimm(i32, i64, i64)
declare spir_func float @_Z20work_group_broadcastfmm(float, i64, i64)
declare spir_func i32 @_Z20work_group_broadcastimmm(i32, i64, i64, i64)
declare spir_func float @_Z20work_group_broadcastfmmm(float, i64, i64, i64)
declare spir_func i32 @_Z21work_group_reduce_addi(i32)
declare spir_func float @_Z21work_group_reduce_addf(float)
declare spir_func i32 @_Z21work_group_reduce_mini(i32)
declare spir_func i32 @_Z21work_group_reduce_minj(i32)
declare spir_func float @_Z21work_group_reduce_minf(float)
declare spir_func i32 @_Z21work_group_reduce_maxi(i32)
declare spir_func i32 @_Z21work_group_reduce_maxj(i32)
declare spir_func float @_Z21work_group_reduce_maxf(float)
declare spir_func i32 @_Z29work_group_scan_exclusive_addi(i32)
declare spir_func i32 @_Z29work_group_scan_exclusive_addj(i32)
declare spir_func float @_Z29work_group_scan_exclusive_addf(float)
declare spir_func i32 @_Z29work_group_scan_exclusive_mini(i32)
declare spir_func i32 @_Z29work_group_scan_exclusive_minj(i32)
declare spir_func float @_Z29work_group_scan_exclusive_minf(float)
declare spir_func i32 @_Z29work_group_scan_exclusive_maxi(i32)
declare spir_func i32 @_Z29work_group_scan_exclusive_maxj(i32)
declare spir_func float @_Z29work_group_scan_exclusive_maxf(float)
declare spir_func i32 @_Z29work_group_scan_inclusive_addi(i32)
declare spir_func i32 @_Z29work_group_scan_inclusive_addj(i32)
declare spir_func float @_Z29work_group_scan_inclusive_addf(float)
declare spir_func i32 @_Z29work_group_scan_inclusive_mini(i32)
declare spir_func i32 @_Z29work_group_scan_inclusive_minj(i32)
declare spir_func float @_Z29work_group_scan_inclusive_minf(float)
declare spir_func i32 @_Z29work_group_scan_inclusive_maxi(i32)
declare spir_func i32 @_Z29work_group_scan_inclusive_maxj(i32)
declare spir_func float @_Z29work_group_scan_inclusive_maxf(float)

!opencl.ocl.version = !{!0}

!0 = !{i32 3, i32 0}
