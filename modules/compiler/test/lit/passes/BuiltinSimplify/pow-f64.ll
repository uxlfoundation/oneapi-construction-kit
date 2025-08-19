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

define spir_kernel void @pow_whole_numbers(double addrspace(1)* %d, <2 x double> addrspace(1)* %d2) {
entry:
  %0 = load double, double addrspace(1)* %d, align 8
  %call = tail call spir_func double @_Z3powdd(double %0, double 1.000000e+00)
  %call1 = tail call spir_func double @_Z3powdd(double %call, double 0x41DFFFFFFFC00000)
  %call2 = tail call spir_func double @_Z3powdd(double %call1, double 0x41E0000000000000)
  store double %call2, double addrspace(1)* %d, align 8
  %1 = load <2 x double>, <2 x double> addrspace(1)* %d2, align 16
  %call3 = tail call spir_func <2 x double> @_Z3powDv2_dS_(<2 x double> %1, <2 x double> <double 1.000000e+00, double 1.000000e+00>)
  %call5 = tail call spir_func <2 x double> @_Z3powDv2_dS_(<2 x double> %call3, <2 x double> <double 1.000000e+00, double 0x41DFFFFFFFC00000>)
  %call7 = tail call spir_func <2 x double> @_Z3powDv2_dS_(<2 x double> %call5, <2 x double> <double 1.000000e+00, double 0x41E0000000000000>)
  store <2 x double> %call7, <2 x double> addrspace(1)* %d2, align 16
  ret void
}

declare spir_func double @_Z3powdd(double, double)

declare spir_func <2 x double> @_Z3powDv2_dS_(<2 x double>, <2 x double>)

define spir_kernel void @pow_recip_whole_numbers(double addrspace(1)* %d, <2 x double> addrspace(1)* %d2) {
entry:
  %0 = load double, double addrspace(1)* %d, align 8
  %call = tail call spir_func double @_Z3powdd(double %0, double 5.000000e-01)
  %call1 = tail call spir_func double @_Z3powdd(double %call, double 0x3E00000000200000)
  %call2 = tail call spir_func double @_Z3powdd(double %call1, double 0x3E00000000000000)
  store double %call2, double addrspace(1)* %d, align 8
  %1 = load <2 x double>, <2 x double> addrspace(1)* %d2, align 16
  %call3 = tail call spir_func <2 x double> @_Z3powDv2_dS_(<2 x double> %1, <2 x double> <double 1.000000e+00, double 5.000000e-01>)
  %call5 = tail call spir_func <2 x double> @_Z3powDv2_dS_(<2 x double> %call3, <2 x double> <double 1.000000e+00, double 0x3E00000000200000>)
  %call7 = tail call spir_func <2 x double> @_Z3powDv2_dS_(<2 x double> %call5, <2 x double> <double 1.000000e+00, double 0x3E00000000000000>)
  store <2 x double> %call7, <2 x double> addrspace(1)* %d2, align 16
  ret void
}

define spir_kernel void @powr_whole_numbers(double addrspace(1)* %d, <2 x double> addrspace(1)* %d2) {
entry:
  %0 = load double, double addrspace(1)* %d, align 8
  %call = tail call spir_func double @_Z4powrdd(double %0, double 1.000000e+00)
  %call1 = tail call spir_func double @_Z4powrdd(double %call, double 0x41DFFFFFFFC00000)
  %call2 = tail call spir_func double @_Z4powrdd(double %call1, double 0x41E0000000000000)
  store double %call2, double addrspace(1)* %d, align 8
  %1 = load <2 x double>, <2 x double> addrspace(1)* %d2, align 16
  %call3 = tail call spir_func <2 x double> @_Z4powrDv2_dS_(<2 x double> %1, <2 x double> <double 1.000000e+00, double 1.000000e+00>)
  %call5 = tail call spir_func <2 x double> @_Z4powrDv2_dS_(<2 x double> %call3, <2 x double> <double 1.000000e+00, double 0x41DFFFFFFFC00000>)
  %call7 = tail call spir_func <2 x double> @_Z4powrDv2_dS_(<2 x double> %call5, <2 x double> <double 1.000000e+00, double 0x41E0000000000000>)
  store <2 x double> %call7, <2 x double> addrspace(1)* %d2, align 16
  ret void
}

declare spir_func double @_Z4powrdd(double, double)

declare spir_func <2 x double> @_Z4powrDv2_dS_(<2 x double>, <2 x double>)

; CHECK: define {{(dso_local )?}}spir_kernel void @pow_whole_numbers
; CHECK: call spir_func double @_Z4powndi
; CHECK: call spir_func double @_Z4powndi
; CHECK: call spir_func double @_Z3powdd
; CHECK: call spir_func <2 x double> @_Z4pownDv2_dDv2_i
; CHECK: call spir_func <2 x double> @_Z4pownDv2_dDv2_i
; CHECK: call spir_func <2 x double> @_Z3powDv2_dS_
; CHECK: define {{(dso_local )?}}spir_kernel void @pow_recip_whole_numbers
; CHECK: call spir_func double @_Z5rootndi
; CHECK: call spir_func double @_Z5rootndi
; CHECK: call spir_func double @_Z3powdd
; CHECK: call spir_func <2 x double> @_Z5rootnDv2_dDv2_i
; CHECK: call spir_func <2 x double> @_Z5rootnDv2_dDv2_i
; CHECK: call spir_func <2 x double> @_Z3powDv2_dS_
; CHECK: define {{(dso_local )?}}spir_kernel void @powr_whole_numbers
; CHECK: call spir_func double @_Z4powndi
; CHECK: call spir_func double @_Z4powndi
; CHECK: call spir_func double @_Z4powrdd
; CHECK: call spir_func <2 x double> @_Z4pownDv2_dDv2_i
; CHECK: call spir_func <2 x double> @_Z4pownDv2_dDv2_i
; CHECK: call spir_func <2 x double> @_Z4powrDv2_dS_
