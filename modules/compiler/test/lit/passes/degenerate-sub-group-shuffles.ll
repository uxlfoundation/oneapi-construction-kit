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

; RUN: muxc --passes degenerate-sub-groups,verify < %s | FileCheck %s

; Check that the DegenerateSubGroupPass correctly replaces sub-group
; builtins with work-group collective calls.

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define spir_kernel void @kernel(i32 %x) #0
; CHECK: call i32 @__mux_get_sub_group_local_id()
; CHECK: call i32 @__mux_sub_group_shuffle_i32(i32 %x, i32 %lid)
define spir_kernel void @kernel(i32 %x) #0 {
entry:
  %lid = call i32 @__mux_get_sub_group_local_id()
  %call = call i32 @__mux_sub_group_shuffle_i32(i32 %x, i32 %lid)
  ret void
}

; CHECK-LABEL: define spir_func i32 @linear_id_helper() {
; CHECK: %lid = call i32 @__mux_get_sub_group_local_id()
define spir_func i32 @linear_id_helper() {
entry:
  %lid = call i32 @__mux_get_sub_group_local_id()
  ret i32 %lid
}

; CHECK-LABEL: define spir_func i32 @shuffle_helper(i32 %x) {
; CHECK: = call i32 @linear_id_helper()
; CHECK: = call i32 @__mux_sub_group_shuffle_i32(i32 %x, i32 %lid)
define spir_func i32 @shuffle_helper(i32 %x) {
entry:
  %lid = call i32 @linear_id_helper()
  %shuffle = call i32 @__mux_sub_group_shuffle_i32(i32 %x, i32 %lid)
  ret i32 %shuffle
}

; CHECK-LABEL: define spir_kernel void @kernel_caller(i32 %x) #0 {
; CHECK:  %call = call i32 @shuffle_helper(i32 %x)
define spir_kernel void @kernel_caller(i32 %x) #0 {
entry:
  %call = call i32 @shuffle_helper(i32 %x)
  ret void
}

; CHECK-LABEL: define spir_kernel void @degenerate_caller.degenerate-subgroups(ptr %out) #1 {
; CHECK:  %call = call i32 @linear_id_helper.degenerate-subgroups()
; CHECK: }

; CHECK-LABEL: define spir_func i32 @linear_id_helper.degenerate-subgroups() #2 {
; CHECK: %0 = call i64 @__mux_get_local_linear_id()
; CHECK: %1 = trunc i64 %0 to i32
define spir_kernel void @degenerate_caller(ptr %out) #0 {
entry:
  %call = call i32 @linear_id_helper()
  store i32 %call, ptr %out
  ret void
}

declare i32 @__mux_get_sub_group_local_id()
declare i32 @__mux_sub_group_shuffle_i32(i32, i32)

attributes #0 = { "mux-kernel"="entry-point" }

; CHECK: attributes #0 = { "mux-kernel"="entry-point" }
; CHECK: attributes #1 = { "mux-base-fn-name"="degenerate_caller" "mux-degenerate-subgroups" "mux-kernel"="entry-point" }
; CHECK: attributes #2 = { "mux-base-fn-name"="linear_id_helper" }
