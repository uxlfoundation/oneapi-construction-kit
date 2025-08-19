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

; RUN: muxc --passes builtin-simplify,verify -S %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @sin_no_fold(double addrspace(1)* %d) {
entry:
  %0 = load double, double addrspace(1)* %d, align 8
  %call = tail call spir_func double @_Z3sind(double %0)
  store double %call, double addrspace(1)* %d, align 8
  ret void
}

declare spir_func double @_Z3sind(double)

define spir_kernel void @sin_fold(double addrspace(1)* %d, <2 x double> addrspace(1)* %d2) {
entry:
  %call = tail call spir_func double @_Z3sind(double 1.000000e+00)
  store double %call, double addrspace(1)* %d, align 8
  %call1 = tail call spir_func <2 x double> @_Z3sinDv2_d(<2 x double> <double 1.000000e+00, double 2.000000e+00>)
  store <2 x double> %call1, <2 x double> addrspace(1)* %d2, align 16
  ret void
}

declare spir_func <2 x double> @_Z3sinDv2_d(<2 x double>)

define spir_kernel void @half_sin_no_fold(double addrspace(1)* %d) {
entry:
  %0 = load double, double addrspace(1)* %d, align 8
  %call = tail call spir_func double @_Z8half_sind(double %0)
  store double %call, double addrspace(1)* %d, align 8
  ret void
}

declare spir_func double @_Z8half_sind(double)

define spir_kernel void @native_sin_no_fold(double addrspace(1)* %d) {
entry:
  %0 = load double, double addrspace(1)* %d, align 8
  %call = tail call spir_func double @_Z10native_sind(double %0)
  store double %call, double addrspace(1)* %d, align 8
  ret void
}

declare spir_func double @_Z10native_sind(double)

define spir_kernel void @sin_double_call(double addrspace(1)* %d) {
entry:
  %call = tail call spir_func double @_Z3sind(double 1.000000e+00)
  %call1 = tail call spir_func double @_Z3sind(double %call)
  store double %call1, double addrspace(1)* %d, align 8
  ret void
}

; CHECK: define {{(dso_local )?}}spir_kernel void @sin_no_fold
; CHECK: call spir_func double @_Z3sind
; CHECK: define {{(dso_local )?}}spir_kernel void @sin_fold
; CHECK-NOT: call spir_func double @_Z3sind
; CHECK-NOT: call spir_func <2 x double> @_Z3sinDv2_d
; CHECK: define {{(dso_local )?}}spir_kernel void @half_sin_no_fold
; CHECK: call spir_func double @_Z8half_sind
; CHECK: define {{(dso_local )?}}spir_kernel void @native_sin_no_fold
; CHECK: call spir_func double @_Z10native_sind
; CHECK: define {{(dso_local )?}}spir_kernel void @sin_double_call
; CHECK-NOT: call spir_func double @_Z3sind
