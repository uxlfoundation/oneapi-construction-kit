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

; RUN: muxc --passes manual-type-legalization,verify -S %s | FileCheck %s

target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux-gnu-elf"

; CHECK-LABEL: @sitofp_i32_to_f32
; CHECK: sitofp <2 x i32> %0 to <2 x float>
define <2 x float> @sitofp_i32_to_f32(<2 x i32> %0) {
entry:
  %1 = sitofp <2 x i32> %0 to <2 x float>
  ret <2 x float> %1
}

; CHECK-LABEL: @sitofp_i32_to_f64
; CHECK: sitofp <2 x i32> %0 to <2 x double>
define <2 x double> @sitofp_i32_to_f64(<2 x i32> %0) {
entry:
  %1 = sitofp <2 x i32> %0 to <2 x double>
  ret <2 x double> %1
}

; CHECK-LABEL: @sitofp_i64_to_f32
; CHECK-DAG: [[ELT0:%.+]] = extractelement <2 x i64> %0, i64 0
; CHECK-DAG: [[ELT1:%.+]] = extractelement <2 x i64> %0, i64 1
; CHECK-DAG: [[CVT0:%.+]] = sitofp i64 [[ELT0]] to float
; CHECK-DAG: [[CVT1:%.+]] = sitofp i64 [[ELT1]] to float
; CHECK-NOT: sitofp <2 x i64>
define <2 x float> @sitofp_i64_to_f32(<2 x i64> %0) {
entry:
  %1 = sitofp <2 x i64> %0 to <2 x float>
  ret <2 x float> %1
}

; CHECK-LABEL: @sitofp_i64_to_f64
; CHECK: sitofp <2 x i64> %0 to <2 x double>
define <2 x double> @sitofp_i64_to_f64(<2 x i64> %0) {
entry:
  %1 = sitofp <2 x i64> %0 to <2 x double>
  ret <2 x double> %1
}

; CHECK-LABEL: @uitofp_i32_to_f32
; CHECK: uitofp <2 x i32> %0 to <2 x float>
define <2 x float> @uitofp_i32_to_f32(<2 x i32> %0) {
entry:
  %1 = uitofp <2 x i32> %0 to <2 x float>
  ret <2 x float> %1
}

; CHECK-LABEL: @uitofp_i32_to_f64
; CHECK: uitofp <2 x i32> %0 to <2 x double>
define <2 x double> @uitofp_i32_to_f64(<2 x i32> %0) {
entry:
  %1 = uitofp <2 x i32> %0 to <2 x double>
  ret <2 x double> %1
}

; CHECK-LABEL: @uitofp_i64_to_f32
; CHECK-DAG: [[ELT0:%.+]] = extractelement <2 x i64> %0, i64 0
; CHECK-DAG: [[ELT1:%.+]] = extractelement <2 x i64> %0, i64 1
; CHECK-DAG: [[CVT0:%.+]] = uitofp i64 [[ELT0]] to float
; CHECK-DAG: [[CVT1:%.+]] = uitofp i64 [[ELT1]] to float
; CHECK-NOT: uitofp <2 x i64>
define <2 x float> @uitofp_i64_to_f32(<2 x i64> %0) {
entry:
  %1 = uitofp <2 x i64> %0 to <2 x float>
  ret <2 x float> %1
}

; CHECK-LABEL: @uitofp_i64_to_f64
; CHECK: uitofp <2 x i64> %0 to <2 x double>
define <2 x double> @uitofp_i64_to_f64(<2 x i64> %0) {
entry:
  %1 = uitofp <2 x i64> %0 to <2 x double>
  ret <2 x double> %1
}
