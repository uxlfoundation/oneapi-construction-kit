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

; Check that the DegenerateSubGroupPass correctly clones a kernel to create
; a degenerate and a non-degenerate subgroup version, and replaces sub-group
; builtins with work-group collective calls in the degenerate version.

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define spir_func i32 @sub_group_reduce_add_test
; CHECK: (i32 [[X:%.*]]) #[[ATTR1:[0-9]+]]
; CHECK: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @__mux_sub_group_reduce_add_i32(i32 [[X]])
; CHECK: ret i32 [[RESULT]]
; CHECK: }
define spir_func i32 @sub_group_reduce_add_test(i32 %x) #0 {
entry:
  %call = call spir_func i32 @__mux_sub_group_reduce_add_i32(i32 %x)
  ret i32 %call
}


; CHECK-LABEL: define spir_func i32 @sub_group_reduce_add_test.degenerate-subgroups
; CHECK: (i32 [[Y:%.*]]) #[[ATTR0:[0-9]+]]
; CHECK: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @__mux_work_group_reduce_add_i32(i32 0, i32 [[Y]])
; CHECK: ret i32 [[RESULT]]
; CHECK: }

; CHECK: declare spir_func i32 @__mux_work_group_reduce_add_i32(i32, i32)

declare spir_func i32 @__mux_sub_group_reduce_add_i32(i32)

attributes #0 = { "mux-kernel"="entry-point" }

!opencl.ocl.version = !{!0}

!0 = !{i32 3, i32 0}

; CHECK-DAG: attributes #[[ATTR0]] = { "mux-base-fn-name"="sub_group_reduce_add_test" "mux-degenerate-subgroups" "mux-kernel"="entry-point" }
; CHECK-DAG: attributes #[[ATTR1]] = { "mux-kernel"="entry-point" }
