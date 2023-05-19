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

; Check that our printf replacement removes the need for double types, and that
; the combine-fpext-fptrunc pass later cleans them up.
; RUN: %muxc --passes replace-printf,combine-fpext-fptrunc,verify -S %s \
; RUN:   --device-fp64-capabilities=false  | %filecheck %s
; Check that instcombine does the same thing (in this case, at least)
; RUN: %muxc --passes replace-printf,instcombine,verify -S %s \
; RUN:   --device-fp64-capabilities=false  | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

@.str = private unnamed_addr addrspace(2) constant [3 x i8] c"%f\00", align 1
@.str.1 = private unnamed_addr addrspace(2) constant [7 x i8] c"%f, %f\00", align 1

; CHECK-NOT: double
define spir_kernel void @print_float(ptr addrspace(1) %in) {
entry:
  %0 = load float, ptr addrspace(1) %in, align 4
  %conv = fpext float %0 to double
  %call = call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @.str, double %conv)
  %call1 = call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @.str, double 1.000000e+00)
  %1 = load float, ptr addrspace(1) %in, align 4
  %conv2 = fpext float %1 to double
  %call4 = call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @.str.1, double %conv2, double %conv2)
  %call8 = call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @.str, double %conv2, double %conv2, double %conv2)
  ret void
}

declare spir_func i32 @printf(ptr addrspace(2), ...)
