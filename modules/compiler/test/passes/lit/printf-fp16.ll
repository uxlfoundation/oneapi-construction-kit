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

; Check we extend halfs to either float or double, depending on the device
; capabilities.

; RUN: %muxc --passes replace-printf,verify -S %s \
; RUN:   --device-fp64-capabilities=false  | %filecheck %s --check-prefixes CHECK,EXT2FP32
; RUN: %muxc --passes replace-printf,verify -S %s \
; RUN:   --device-fp64-capabilities=true  | %filecheck %s --check-prefixes CHECK,EXT2FP64

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

@.str = private unnamed_addr addrspace(2) constant [6 x i8] c"%v4hf\00", align 1

; CHECK: define spir_kernel void @print_half(<4 x half> %h, ptr addrspace(1) [[BUFFER:%.*]]) {
; EXT2FP32: call spir_func i32 @{{.*}}(ptr addrspace(1) [[BUFFER]], float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.000000e+00)
; EXT2FP64: call spir_func i32 @{{.*}}(ptr addrspace(1) [[BUFFER]], double 1.000000e+00, double 2.000000e+00, double 3.000000e+00, double 4.000000e+00)
; CHECK-DAG: [[X0:%.*]] = extractelement <4 x half> %h, i32 0
; CHECK-DAG: [[Y0:%.*]] = extractelement <4 x half> %h, i32 1
; CHECK-DAG: [[Z0:%.*]] = extractelement <4 x half> %h, i32 2
; CHECK-DAG: [[W0:%.*]] = extractelement <4 x half> %h, i32 3
; EXT2FP32-DAG: [[X:%.*]] = fpext half [[X0]] to float
; EXT2FP32-DAG: [[Y:%.*]] = fpext half [[Y0]] to float
; EXT2FP32-DAG: [[Z:%.*]] = fpext half [[Z0]] to float
; EXT2FP32-DAG: [[W:%.*]] = fpext half [[W0]] to float
; EXT2FP32: call spir_func i32 @{{.*}}(ptr addrspace(1) [[BUFFER]], float [[X]], float [[Y]], float [[Z]], float [[W]])

; EXT2FP64-DAG: [[X:%.*]] = fpext half [[X0]] to double
; EXT2FP64-DAG: [[Y:%.*]] = fpext half [[Y0]] to double
; EXT2FP64-DAG: [[Z:%.*]] = fpext half [[Z0]] to double
; EXT2FP64-DAG: [[W:%.*]] = fpext half [[W0]] to double
; EXT2FP64: call spir_func i32 @{{.*}}(ptr addrspace(1) [[BUFFER]], double [[X]], double [[Y]], double [[Z]], double [[W]])

define spir_kernel void @print_half(<4 x half> %h) {
entry:
  %call1 = call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @.str, <4 x half> <half 1.0, half 2.0, half 3.0, half 4.0>)
  %call2 = call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @.str, <4 x half> %h)
  ret void
}

declare spir_func i32 @printf(ptr addrspace(2), ...)
