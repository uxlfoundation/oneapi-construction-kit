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

; RUN: muxc --passes replace-printf,verify -S %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

@.str = private unnamed_addr addrspace(2) constant [10 x i8] c"id = %lu\0A\00", align 1

; CHECK-LABEL: define spir_kernel void @do_printf(
; CHECK: %id = call spir_func i64 @_Z13get_global_idj(i32 0)
; CHECK: = call spir_func i32 @0(ptr addrspace(1) %0, i64 %id)
define spir_kernel void @do_printf(i32 addrspace(1)* %a) {
entry:
  %id = call spir_func i64 @_Z13get_global_idj(i32 0)
  %call1 = tail call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* getelementptr inbounds ([10 x i8], [10 x i8] addrspace(2)* @.str, i64 0, i64 0), i64 %id)
  ret void
}

; CHECK-LABEL: define linkonce_odr spir_func i32 @0(
; CHECK-LABEL: store:
; Store the printf call ID
; CHECK: [[ID_PTR:%.*]] = getelementptr i8, ptr addrspace(1) [[PTR:%.*]], i32 [[IDX:%.*]]
; CHECK: store i32 0, ptr addrspace(1) [[ID_PTR]]


; Store the argument we're printing at the next address
; CHECK: [[NEXT_IDX:%.*]] = add i32 [[IDX]], 4
; CHECK: [[VAL_PTR:%.*]] = getelementptr i8, ptr addrspace(1) [[PTR]], i32 [[NEXT_IDX]]
; CHECK: store i64 %1, ptr addrspace(1) [[VAL_PTR]]

declare spir_func i64 @_Z13get_global_idj(i32)

declare spir_func i32 @printf(i8 addrspace(2)*, ...)
