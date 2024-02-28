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

; Make sure we use a triple that does not have half as a legal type.
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare half @llvm.fma.f16(half, half, half)

; CHECK-LABEL: define half @fadd
; CHECK-DAG: [[AEXT:%.*]] = fpext half %a to float
; CHECK-DAG: [[BEXT:%.*]] = fpext half %b to float
; CHECK-DAG: [[CEXT:%.*]] = fpext half %c to float
; CHECK-DAG: [[DADD:%.*]] = fadd float [[AEXT]], [[BEXT]]
; CHECK-DAG: [[DTRUNC:%.*]] = fptrunc float [[DADD]] to half
; CHECK-DAG: [[DEXT:%.*]] = fpext half [[DTRUNC]] to float
; CHECK-DAG: [[EADD:%.*]] = fadd float [[DEXT]], [[CEXT]]
; CHECK-DAG: [[ETRUNC:%.*]] = fptrunc float [[EADD]] to half
; CHECK: ret half [[ETRUNC]]
define half @fadd(half %a, half %b, half %c) {
entry:
  %d = fadd half %a, %b
  %e = fadd half %d, %c
  ret half %e
}

; CHECK-LABEL: define half @ffma
; CHECK-DAG: [[AEXT:%.*]] = fpext half %a to double
; CHECK-DAG: [[BEXT:%.*]] = fpext half %b to double
; CHECK-DAG: [[CEXT:%.*]] = fpext half %c to double
; CHECK-DAG: [[DFMA:%.*]] = call double @llvm.fmuladd.f64(double [[AEXT]], double [[BEXT]], double [[CEXT]])
; CHECK-DAG: [[DTRUNC:%.*]] = fptrunc double [[DADD]] to half
; CHECK: ret half [[DTRUNC]]
define half @ffma(half %a, half %b, half %c) {
entry:
  %d = call half @llvm.fma.f16(half %a, half %b, half %c)
  ret half %d
}

; CHECK-LABEL: define half @ffmaadd
; CHECK-DAG: [[AEXTD:%.*]] = fpext half %a to double
; CHECK-DAG: [[BEXTD:%.*]] = fpext half %b to double
; CHECK-DAG: [[CEXTD:%.*]] = fpext half %c to double
; CHECK-DAG: [[DFMAD:%.*]] = call double @llvm.fmuladd.f64(double [[AEXTD]], double [[BEXTD]], double [[CEXTD]])
; CHECK-DAG: [[DTRUNC:%.*]] = fptrunc double [[DADD]] to half
; CHECK-DAG: [[CEXTF:%.*]] = fpext half %c to float
; CHECK-DAG: [[DEXTF:%.*]] = fpext half %d to float
; CHECK-DAG: [[EADD:%.*]] = fadd float [[DEXTF]], [[CEXTF]]
; CHECK-DAG: [[ETRUNC:%.*]] = fptrunc float [[EADD]] to half
; CHECK: ret half [[ETRUNC]]
define half @ffmaadd(half %a, half %b, half %c) {
entry:
  %d = call half @llvm.fma.f16(half %a, half %b, half %c)
  %e = fadd half %d, %c
  ret half %e
}
