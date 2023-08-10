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
; RUN: muxc --passes degenerate-sub-groups,verify -S %s | FileCheck %s

; Check that the DegenerateSubGroupPass correctly replaces sub-group
; broadcasts with work-group broadcasts using the correct mangling of size_t on
; a 32 bit system.

target datalayout = "e-i64:64-p:32:32-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir32-unknown-unknown"

; CHECK: define spir_func i32 @sub_group_broadcast_test(i32 [[VAL:%.*]], i32 [[LID:%.*]])
define spir_func i32 @sub_group_broadcast_test(i32 %val, i32 %lid) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[LSX:%.*]] = call i32 @__mux_get_local_size(i32 0)
; CHECK: [[LSY:%.*]] = call i32 @__mux_get_local_size(i32 1)
; CHECK: [[X:%.*]] = urem i32 [[LID]], [[LSX]]
; CHECK: [[LIDSUBLSX:%.*]] = sub i32 [[LID]], [[X]]
; CHECK: [[LIDSUBLSXDI:%.*]] = udiv i32 [[LIDSUBLSX]], [[LSX]]
; CHECK: [[Y:%.*]] = urem i32 [[LIDSUBLSXDI]], [[LSY]]
; CHECK-DAG: [[LSXLSY:%.*]] = mul i32 [[LSX]], [[LSY]]
; CHECK-DAG: [[YLSX:%.*]] = mul i32 [[Y]], [[LSX]]
; CHECK-DAG: [[XADDYLSX:%.*]] = add i32 [[X]], [[YLSX]]
; CHECK-DAG: [[LIDSUBXADDYLSX:%.*]] = sub i32 [[LID]], [[XADDYLSX]]
; CHECK: [[Z:%.*]] = udiv i32 [[LIDSUBXADDYLSX]], [[LSXLSY]]
; CHECK: [[RESULT:%.*]] = call i32 @__mux_work_group_broadcast_i32(i32 0, i32 [[VAL]], i32 [[X]], i32 [[Y]], i32 [[Z]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @__mux_sub_group_broadcast_i32(i32 %val, i32 %lid)
  ret i32 %call
}

attributes #0 = { "mux-kernel"="entry-point" }

!opencl.ocl.version = !{!1}

!0 = !{i32 13, i32 64, i32 64}
!1 = !{i32 3, i32 0}

; CHECK: declare spir_func i32 @__mux_work_group_broadcast_i32(i32, i32, i32, i32, i32)
declare spir_func i32 @__mux_sub_group_broadcast_i32(i32, i32)
