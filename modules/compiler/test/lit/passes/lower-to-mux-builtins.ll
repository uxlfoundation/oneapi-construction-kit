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

; RUN: muxc --passes lower-to-mux-builtins,verify -S %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; Global work-item functions
declare spir_func i64 @_Z15get_global_sizej(i32)
declare spir_func i64 @_Z13get_global_idj(i32)
declare spir_func i64 @_Z17get_global_offsetj(i32)
declare spir_func i64 @_Z14get_num_groupsj(i32)
declare spir_func i64 @_Z12get_group_idj(i32)
declare spir_func i32 @_Z12get_work_dimv()

; Local work-item functions
declare spir_func i64 @_Z14get_local_sizej(i32)
declare spir_func i64 @_Z12get_local_idj(i32)

; OpenCL 2.0+ functions
declare spir_func i64 @_Z19get_local_linear_idv()
declare spir_func i64 @_Z20get_global_linear_idv()

; Sub-groups
declare spir_func i64 @_Z23get_enqueued_local_sizej(i32 %id)
declare spir_func i32 @_Z22get_max_sub_group_sizev()
declare spir_func i32 @_Z18get_num_sub_groupsv()
declare spir_func i32 @_Z27get_enqueued_num_sub_groupsv()
declare spir_func i32 @_Z16get_sub_group_idv()

define void @test_fn() {
; CHECK: %call0 = call i64 @__mux_get_global_size(i32 1)
  %call0 = call spir_func i64 @_Z15get_global_sizej(i32 1)
; CHECK: %call1 = call i64 @__mux_get_global_id(i32 2)
  %call1 = call spir_func i64 @_Z13get_global_idj(i32 2)
; CHECK: %call2 = call i64 @__mux_get_global_offset(i32 0)
  %call2 = call spir_func i64 @_Z17get_global_offsetj(i32 0)
; CHECK: %call3 = call i64 @__mux_get_num_groups(i32 0)
  %call3 = call spir_func i64 @_Z14get_num_groupsj(i32 0)
; CHECK: %call4 = call i64 @__mux_get_group_id(i32 1)
  %call4 = call spir_func i64 @_Z12get_group_idj(i32 1)
; CHECK: %call5 = call i32 @__mux_get_work_dim()
  %call5 = call spir_func i32 @_Z12get_work_dimv()
; CHECK: %call6 = call i64 @__mux_get_local_size(i32 2)
  %call6 = call spir_func i64 @_Z14get_local_sizej(i32 2)
; CHECK: %call7 = call i64 @__mux_get_local_id(i32 1)
  %call7 = call spir_func i64 @_Z12get_local_idj(i32 1)
; CHECK: %call8 = call i64 @__mux_get_local_linear_id()
  %call8 = call spir_func i64 @_Z19get_local_linear_idv()
; CHECK: %call9 = call i64 @__mux_get_global_linear_id()
  %call9 = call spir_func i64 @_Z20get_global_linear_idv()
; CHECK: %call10 = call i64 @__mux_get_enqueued_local_size(i32 0)
  %call10 = call spir_func i64 @_Z23get_enqueued_local_sizej(i32 0)
; CHECK: %call11 = call i32 @__mux_get_max_sub_group_size()
  %call11 = call spir_func i32 @_Z22get_max_sub_group_sizev()
; CHECK: %call12 = call i32 @__mux_get_num_sub_groups()
  %call12 = call spir_func i32 @_Z18get_num_sub_groupsv()
; Note - same builtin as we don't support non-uniform work-group sizes
; CHECK: %call13 = call i32 @__mux_get_num_sub_groups()
  %call13 = call spir_func i32 @_Z27get_enqueued_num_sub_groupsv()
; CHECK: %call14 = call i32 @__mux_get_sub_group_id()
  %call14 = call spir_func i32 @_Z16get_sub_group_idv()
  ret void
}

!opencl.ocl.version = !{!0}
!0 = !{i32 3, i32 0}
