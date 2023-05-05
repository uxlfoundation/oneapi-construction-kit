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

; RUN: %muxc --passes builtin-simplify,verify -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @log10_no_fold(double addrspace(1)* %d) {
entry:
  %0 = load double, double addrspace(1)* %d, align 8
  %call = tail call spir_func double @_Z5log10d(double %0)
  store double %call, double addrspace(1)* %d, align 8
  ret void
}

declare spir_func double @_Z5log10d(double)

define spir_kernel void @log10_fold(double addrspace(1)* %d, <2 x double> addrspace(1)* %d2) {
entry:
  %call = tail call spir_func double @_Z5log10d(double 1.000000e+00)
  store double %call, double addrspace(1)* %d, align 8
  %call1 = tail call spir_func <2 x double> @_Z5log10Dv2_d(<2 x double> <double 1.000000e+00, double 2.000000e+00>)
  store <2 x double> %call1, <2 x double> addrspace(1)* %d2, align 16
  ret void
}

declare spir_func <2 x double> @_Z5log10Dv2_d(<2 x double>)

; CHECK: define {{(dso_local )?}}spir_kernel void @log10_no_fold
; CHECK: call spir_func double @_Z5log10d
; CHECK: define {{(dso_local )?}}spir_kernel void @log10_fold
; CHECK-NOT: call spir_func double @_Z5log10d
; CHECK-NOT: call spir_func <2 x double> @_Z5log10Dv2_d
