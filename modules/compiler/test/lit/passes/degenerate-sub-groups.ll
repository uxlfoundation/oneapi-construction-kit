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
; builtins with work-group collective calls.

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

; CHECK: define spir_func i1 @sub_group_all_test(i1 [[X:%.*]])
define spir_func i1 @sub_group_all_test(i1 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i1 @__mux_work_group_all_i1(i32 0, i1 [[X]])
; CHECK: ret i1 [[RESULT]]
entry:
  %call = call spir_func i1 @__mux_sub_group_all_i1(i1 %x)
  ret i1 %call
}

; CHECK: define spir_func i1 @sub_group_any_test(i1 [[X:%.*]])
define spir_func i1 @sub_group_any_test(i1 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i1 @__mux_work_group_any_i1(i32 0, i1 [[X]])
; CHECK: ret i1 [[RESULT]]
entry:
  %call = call spir_func i1 @__mux_sub_group_any_i1(i1 %x)
  ret i1 %call
}

; CHECK: define spir_func i32 @sub_group_broadcast_test(i32 [[VAL:%.*]], i32 [[LID:%.*]])
define spir_func i32 @sub_group_broadcast_test(i32 %val, i32 %lid) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[LSXi64:%.*]] = call i64 @__mux_get_local_size(i32 0)
; CHECK: [[LSX:%.*]] = trunc i64 [[LSXi64]] to i32
; CHECK: [[LSYi64:%.*]] = call i64 @__mux_get_local_size(i32 1)
; CHECK: [[LSY:%.*]] = trunc i64 [[LSYi64]] to i32
; CHECK: [[X:%.*]] = urem i32 [[LID]], [[LSX]]
; CHECK: [[LIDSUBLSX:%.*]] = sub i32 [[LID]], [[X]]
; CHECK: [[LIDSUBLSXDI:%.*]] = udiv i32 [[LIDSUBLSX]], [[LSX]]
; CHECK: [[Y:%.*]] = urem i32 [[LIDSUBLSXDI]], [[LSY]]
; CHECK-DAG: [[LSXLSY:%.*]] = mul i32 [[LSX]], [[LSY]]
; CHECK-DAG: [[YLSX:%.*]] = mul i32 [[Y]], [[LSX]]
; CHECK-DAG: [[XADDYLSX:%.*]] = add i32 [[X]], [[YLSX]]
; CHECK-DAG: [[LIDSUBXADDYLSX:%.*]] = sub i32 [[LID]], [[XADDYLSX]]
; CHECK: [[Z:%.*]] = udiv i32 [[LIDSUBXADDYLSX]], [[LSXLSY]]
; CHECK: [[Xi64:%.*]] = zext i32 [[X]] to i64
; CHECK: [[Yi64:%.*]] = zext i32 [[Y]] to i64
; CHECK: [[Zi64:%.*]] = zext i32 [[Z]] to i64
; CHECK: [[RESULT:%.*]] = call i32 @__mux_work_group_broadcast_i32(i32 0, i32 [[VAL]], i64 [[Xi64]], i64 [[Yi64]], i64 [[Zi64]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @__mux_sub_group_broadcast_i32(i32 %val, i32 %lid)
  ret i32 %call
}

; CHECK: define spir_func i32 @sub_group_reduce_add_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_reduce_add_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @__mux_work_group_reduce_add_i32(i32 0, i32 [[X]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @__mux_sub_group_reduce_add_i32(i32 %x)
  ret i32 %call
}

; CHECK: define spir_func i32 @sub_group_reduce_min_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_reduce_min_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @__mux_work_group_reduce_smin_i32(i32 0, i32 [[X]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @__mux_sub_group_reduce_smin_i32(i32 %x)
  ret i32 %call
}

; CHECK: define spir_func i32 @sub_group_reduce_max_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_reduce_max_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @__mux_work_group_reduce_smax_i32(i32 0, i32 [[X]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @__mux_sub_group_reduce_smax_i32(i32 %x)
  ret i32 %call
}

; CHECK: define spir_func i32 @sub_group_scan_exclusive_add_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_scan_exclusive_add_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @__mux_work_group_scan_exclusive_add_i32(i32 0, i32 [[X]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @__mux_sub_group_scan_exclusive_add_i32(i32 %x)
  ret i32 %call
}

; CHECK: define spir_func i32 @sub_group_scan_exclusive_min_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_scan_exclusive_min_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @__mux_work_group_scan_exclusive_smin_i32(i32 0, i32 [[X]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @__mux_sub_group_scan_exclusive_smin_i32(i32 %x)
  ret i32 %call
}

; CHECK: define spir_func i32 @sub_group_scan_exclusive_max_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_scan_exclusive_max_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @__mux_work_group_scan_exclusive_smax_i32(i32 0, i32 [[X]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @__mux_sub_group_scan_exclusive_smax_i32(i32 %x)
  ret i32 %call
}
; CHECK: define spir_func i32 @sub_group_scan_inclusive_add_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_scan_inclusive_add_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @__mux_work_group_scan_inclusive_add_i32(i32 0, i32 [[X]])
entry:
  %call = call spir_func i32 @__mux_sub_group_scan_inclusive_add_i32(i32 %x)
  ret i32 %call
}

; CHECK: define spir_func i32 @sub_group_scan_inclusive_min_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_scan_inclusive_min_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @__mux_work_group_scan_inclusive_smin_i32(i32 0, i32 [[X]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @__mux_sub_group_scan_inclusive_smin_i32(i32 %x)
  ret i32 %call
}

; CHECK: define spir_func i32 @sub_group_scan_inclusive_max_test(i32 [[X:%.*]])
define spir_func i32 @sub_group_scan_inclusive_max_test(i32 %x) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[RESULT:%.*]] = call spir_func i32 @__mux_work_group_scan_inclusive_smax_i32(i32 0, i32 [[X]])
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @__mux_sub_group_scan_inclusive_smax_i32(i32 %x)
  ret i32 %call
}

; CHECK: define spir_func i32 @get_sub_group_size_test()
define spir_func i32 @get_sub_group_size_test() #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[X:%.*]] = call spir_func i64 @__mux_get_local_size(i32 0)
; CHECK: [[LOCALSIZETMPA:%.*]] = mul i64 [[X]], 1
; CHECK: [[Y:%.*]] = call spir_func i64 @__mux_get_local_size(i32 1)
; CHECK: [[LOCALSIZETMPB:%.*]] = mul i64 [[Y]], [[LOCALSIZETMPA]]
; CHECK: [[Z:%.*]] = call spir_func i64 @__mux_get_local_size(i32 2)
; CHECK: [[LOCALSIZE:%.*]] = mul i64 [[Z]], [[LOCALSIZETMPB]]
; CHECK: [[RESULT:%.*]] = trunc i64 [[LOCALSIZE]] to i32
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @__mux_get_sub_group_size()
  ret i32 %call
}

; CHECK: define spir_func i32 @get_max_sub_group_size_test()
define spir_func i32 @get_max_sub_group_size_test() #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[X:%.*]] = call spir_func i64 @__mux_get_local_size(i32 0)
; CHECK: [[LOCALSIZETMPA:%.*]] = mul i64 [[X]], 1
; CHECK: [[Y:%.*]] = call spir_func i64 @__mux_get_local_size(i32 1)
; CHECK: [[LOCALSIZETMPB:%.*]] = mul i64 [[Y]], [[LOCALSIZETMPA]]
; CHECK: [[Z:%.*]] = call spir_func i64 @__mux_get_local_size(i32 2)
; CHECK: [[LOCALSIZE:%.*]] = mul i64 [[Z]], [[LOCALSIZETMPB]]
; CHECK: [[RESULT:%.*]] = trunc i64 [[LOCALSIZE]] to i32
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @__mux_get_max_sub_group_size()
  ret i32 %call
}

; CHECK: define spir_func i32 @get_num_sub_groups_test()
define spir_func i32 @get_num_sub_groups_test() #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: ret i32 1
entry:
  %call = call spir_func i32 @__mux_get_num_sub_groups()
  ret i32 %call
}

; CHECK: define spir_func i32 @get_sub_group_id_test()
define spir_func i32 @get_sub_group_id_test() #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: ret i32 0
entry:
  %call = call spir_func i32 @__mux_get_sub_group_id()
  ret i32 %call
}

; CHECK: define spir_func i32 @get_sub_group_local_id_test()
define spir_func i32 @get_sub_group_local_id_test() #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: [[LLID:%.*]] = call spir_func i64 @__mux_get_local_linear_id()
; CHECK: [[RESULT:%.*]] = trunc i64 [[LLID]] to i32
; CHECK: ret i32 [[RESULT]]
entry:
  %call = call spir_func i32 @__mux_get_sub_group_local_id()
  ret i32 %call
}

; CHECK: define spir_func void @sub_group_barrier_test(i32 [[FLAGS:%.*]], i32 [[SCOPE:%.*]])
define spir_func void @sub_group_barrier_test(i32 %flags, i32 %scope) #0 !reqd_work_group_size !0 {
; CHECK-LABEL: entry:
; CHECK: call spir_func void @__mux_work_group_barrier(i32 -1, i32 [[FLAGS]], i32 [[SCOPE]])
; CHECK: ret void
entry:
  call spir_func void @__mux_sub_group_barrier(i32 -1, i32 %flags, i32 %scope)
  ret void
}

; CHECK: define spir_func void @no_sub_groups_test() [[ATTRS:#[0-9]+]] {
define spir_func void @no_sub_groups_test() #1 {
  ret void
}

; CHECK-DAG: declare spir_func i1 @__mux_work_group_all_i1(i32, i1)
declare spir_func i1 @__mux_sub_group_all_i1(i1)
; CHECK-DAG: declare spir_func i1 @__mux_work_group_any_i1(i32, i1)
declare spir_func i1 @__mux_sub_group_any_i1(i1)
; CHECK-DAG: declare spir_func i32 @__mux_work_group_broadcast_i32(i32, i32, i64, i64, i64)
declare spir_func i32 @__mux_sub_group_broadcast_i32(i32, i32)
; CHECK-DAG: declare spir_func i32 @__mux_work_group_reduce_add_i32(i32, i32)
declare spir_func i32 @__mux_sub_group_reduce_add_i32(i32)
; CHECK-DAG: declare spir_func i32 @__mux_work_group_reduce_smin_i32(i32, i32)
declare spir_func i32 @__mux_sub_group_reduce_smin_i32(i32)
; CHECK-DAG: declare spir_func i32 @__mux_work_group_reduce_smax_i32(i32, i32)
declare spir_func i32 @__mux_sub_group_reduce_smax_i32(i32)
; CHECK-DAG: declare spir_func i32 @__mux_work_group_scan_exclusive_add_i32(i32, i32)
declare spir_func i32 @__mux_sub_group_scan_exclusive_add_i32(i32)
; CHECK-DAG: declare spir_func i32 @__mux_work_group_scan_exclusive_smin_i32(i32, i32)
declare spir_func i32 @__mux_sub_group_scan_exclusive_smin_i32(i32)
; CHECK-DAG: declare spir_func i32 @__mux_work_group_scan_exclusive_smax_i32(i32, i32)
declare spir_func i32 @__mux_sub_group_scan_exclusive_smax_i32(i32)
; CHECK-DAG: declare spir_func i32 @__mux_work_group_scan_inclusive_add_i32(i32, i32)
declare spir_func i32 @__mux_sub_group_scan_inclusive_add_i32(i32)
; CHECK-DAG: declare spir_func i32 @__mux_work_group_scan_inclusive_smin_i32(i32, i32)
declare spir_func i32 @__mux_sub_group_scan_inclusive_smin_i32(i32)
; CHECK-DAG: declare spir_func i32 @__mux_work_group_scan_inclusive_smax_i32(i32, i32)
declare spir_func i32 @__mux_sub_group_scan_inclusive_smax_i32(i32)
; CHECK-DAG: declare spir_func i64 @__mux_get_local_size(i32)
declare spir_func i32 @__mux_get_sub_group_size()
declare spir_func i32 @__mux_get_max_sub_group_size()
declare spir_func i32 @__mux_get_num_sub_groups()
declare spir_func i32 @__mux_get_sub_group_id()
; CHECK-DAG: declare spir_func i64 @__mux_get_local_linear_id()
declare spir_func i32 @__mux_get_sub_group_local_id()
; CHECK-DAG: declare spir_func void @__mux_work_group_barrier(i32, i32, i32)
declare spir_func void @__mux_sub_group_barrier(i32, i32, i32)

; Check we didn't mark a function uses no sub-groups as having degenerate
; sub-groups.
; CHECK-DAG: attributes [[ATTRS]] = { "mux-kernel"="entry-point" "mux-no-subgroups" }
; CHECK-DAG: attributes #0 = { "mux-degenerate-subgroups" "mux-kernel"="entry-point" }
attributes #0 = { "mux-kernel"="entry-point" }
attributes #1 = { "mux-kernel"="entry-point" "mux-no-subgroups" }

!0 = !{i32 13, i32 64, i32 64}

!opencl.ocl.version = !{!1}

!1 = !{i32 3, i32 0}
