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
;
; Additionally, it checks that a kernel that doesn't use any subgroup functions
; is NOT cloned.
;
; Additionally, it checks that a shared function that doesn't use any subgroup
; functions is also NOT cloned, and remains shared between both degenerate and
; non-degenerate kernels.

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

; CHECK: define spir_func i32 @clone_this(i32 [[X6:%.+]]) {
; CHECK: entry:
; CHECK:   [[R6:%.+]] = call spir_func i32 @__mux_sub_group_reduce_add_i32(i32 [[X6]])
; CHECK:   ret i32 [[R6]]
; CHECK: }
define spir_func i32 @clone_this(i32 %x) {
entry:
  %call = call spir_func i32 @__mux_sub_group_reduce_add_i32(i32 %x)
  ret i32 %call
}

; CHECK: define spir_func i32 @shared(i32 [[X2:%.+]]) {
; CHECK: entry:
; CHECK:   [[R2:%.+]] = mul i32 [[X2]], [[X2]]
; CHECK:   ret i32 [[R2]]
; CHECK: }
define spir_func i32 @shared(i32 %x) {
entry:
  %sqr = mul i32 %x, %x
  ret i32 %sqr
}

; CHECK: define spir_func i32 @sub_groups(i32 [[X5:%.+]]) #[[ATTR1:[0-9]+]] {
; CHECK: entry:
; CHECK:   [[C5_1:%.+]] = call spir_func i32 @clone_this(i32 [[X5]])
; CHECK:   [[C5_2:%.+]] = call spir_func i32 @shared(i32 [[X5]])
; CHECK:   [[R5:%.+]] = add i32 [[C5_1]], [[C5_2]]
; CHECK:   ret i32 [[R5]]
; CHECK: }
define spir_func i32 @sub_groups(i32 %x) #0 {
entry:
  %call1 = call spir_func i32 @clone_this(i32 %x)
  %call2 = call spir_func i32 @shared(i32 %x)
  %add = add i32 %call1, %call2
  ret i32 %add
}

; CHECK: define spir_func i32 @no_sub_groups(i32 [[X4:%.+]]) #[[ATTR0:[0-9]+]] {
; CHECK: entry:
; CHECK:   [[R4:%.+]] = call spir_func i32 @shared(i32 [[X4]])
; CHECK:   ret i32 [[R4]]
; CHECK: }
define spir_func i32 @no_sub_groups(i32 %x) #0 {
entry:
  %call = call spir_func i32 @shared(i32 %x)
  ret i32 %call
}

declare spir_func i32 @__mux_sub_group_reduce_add_i32(i32)
; CHECK: declare spir_func i32 @__mux_work_group_reduce_add_i32(i32, i32)

; CHECK: define spir_func i32 @sub_groups.degenerate-subgroups(i32 [[X3:%.+]]) #[[ATTR2:[0-9]+]] {
; CHECK: entry:
; CHECK:   [[C3_1:%.+]] = call spir_func i32 @clone_this.degenerate-subgroups(i32 [[X3]])
; CHECK:   [[C3_2:%.+]] = call spir_func i32 @shared(i32 [[X3]])
; CHECK:   [[R3:%.+]] = add i32 [[C3_1]], [[C3_2]]
; CHECK:   ret i32 [[R3:%.+]]
; CHECK: }

; CHECK: define spir_func i32 @clone_this.degenerate-subgroups(i32 [[X1:%.+]]) #[[ATTR3:[0-9]+]] {
; CHECK: entry:
; CHECK:   [[R1:%.+]] = call spir_func i32 @__mux_work_group_reduce_add_i32(i32 0, i32 [[X1]])
; CHECK:   ret i32 [[R1]]
; CHECK: }

!opencl.ocl.version = !{!0}

!0 = !{i32 3, i32 0}

attributes #0 = { "mux-kernel"="entry-point" }

; CHECK-DAG: attributes #[[ATTR0]] = { "mux-degenerate-subgroups" "mux-kernel"="entry-point" }
; CHECK-DAG: attributes #[[ATTR1]] = { "mux-kernel"="entry-point" }
; CHECK-DAG: attributes #[[ATTR2]] = { "mux-base-fn-name"="sub_groups" "mux-degenerate-subgroups" "mux-kernel"="entry-point" }
; CHECK-DAG: attributes #[[ATTR3]] = { "mux-base-fn-name"="clone_this" }
