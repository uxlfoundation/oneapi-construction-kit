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

; RUN: %muxc --passes add-sched-params,define-mux-builtins,verify -S %s  | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

declare !mux_scheduled_fn !2 i64 @__mux_get_local_size(i32)
; CHECK: define internal i64 @__mux_get_local_size(i32 [[IDX:%.*]], ptr [[WIATTRS:noalias nonnull align 8 dereferenceable\(40\)]] [[WI:%.*]], ptr [[WGATTRS:noalias nonnull align 8 dereferenceable\(104\)]] [[WG:%.*]])
; CHECK:  [[TMP0:%.*]] = icmp ult i32 [[IDX]], 3
; CHECK:  [[TMP1:%.*]] = select i1 [[TMP0]], i32 [[IDX]], i32 0
; CHECK:  [[TMP2:%.*]] = getelementptr %MuxWorkGroupInfo, ptr [[WG]], i32 0, i32 3, i32 [[TMP1]]
; CHECK:  [[TMP3:%.*]] = load i64, ptr [[TMP2]], align 8
; CHECK:  [[TMP4:%.*]] = select i1 [[TMP0]], i64 [[TMP3]], i64 1
; CHECK:  ret i64 [[TMP4]]

; The rest of these simple global lookups are similar
; CHECK: define internal i64 @__mux_get_group_id(
declare !mux_scheduled_fn !2 i64 @__mux_get_group_id(i32)
; CHECK: define internal i64 @__mux_get_num_groups(
declare !mux_scheduled_fn !2 i64 @__mux_get_num_groups(i32)
; CHECK: define internal i64 @__mux_get_global_offset(
declare !mux_scheduled_fn !2 i64 @__mux_get_global_offset(i32)

declare !mux_scheduled_fn !0 i64 @__mux_get_work_dim()
; CHECK: define internal i64 @__mux_get_work_dim(ptr [[WIATTRS]] [[WI:%.*]], ptr [[WGATTRS]] [[WG:%.*]])
; CHECK: [[TMP0:%.*]] = getelementptr %MuxWorkGroupInfo, ptr [[WG]], i32 0, i32 4
; CHECK: [[TMP1:%.*]] = load i64, ptr [[TMP0]], align 8
; CHECK: ret i64 [[TMP1]]

declare !mux_scheduled_fn !1 i64 @__mux_get_global_id(i32)
; CHECK: define internal i64 @__mux_get_global_id(i32 [[IDX:%.*]], ptr [[WIATTRS]] [[WI:%.*]], ptr [[WGATTRS]] [[WG:%.*]])
; CHECK: [[TMP0:%.*]] = call i64 @__mux_get_group_id(i32 [[IDX]], ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK: [[TMP1:%.*]] = call i64 @__mux_get_global_offset(i32 [[IDX]], ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK: [[TMP2:%.*]] = call i64 @__mux_get_local_id(i32 [[IDX]], ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK: [[TMP3:%.*]] = call i64 @__mux_get_local_size(i32 [[IDX]], ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK: [[TMP4:%.*]] = mul i64 [[TMP0]], [[TMP3]]
; CHECK: [[TMP5:%.*]] = add i64 [[TMP4]], [[TMP2]]
; CHECK: [[TMP6:%.*]] = add i64 [[TMP5]], [[TMP1]]
; CHECK: ret i64 [[TMP6]]

declare !mux_scheduled_fn !2 i64 @__mux_get_global_size(i32)
; CHECK: define internal i64 @__mux_get_global_size(i32 [[IDX:%.*]], ptr [[WIATTRS]] [[WI:%.*]], ptr [[WGATTRS]] [[WG:%.*]])
; CHECK: [[TMP0:%.*]] = call i64 @__mux_get_num_groups(i32 [[IDX]], ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK: [[TMP1:%.*]] = call i64 @__mux_get_local_size(i32 [[IDX]], ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK: [[TMP2:%.*]] = mul i64 [[TMP0]], [[TMP1]]
; CHECK: ret i64 [[TMP2]]

declare !mux_scheduled_fn !3 i64 @__mux_get_global_linear_id()
; CHECK: define internal i64 @__mux_get_global_linear_id(ptr [[WIATTRS]] [[WI:%.*]], ptr [[WGATTRS]] [[WG:%.*]])
; CHECK-DAG: [[IDX:%.*]] = call i64 @__mux_get_global_id(i32 0, ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK-DAG: [[IDY:%.*]] = call i64 @__mux_get_global_id(i32 1, ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK-DAG: [[IDZ:%.*]] = call i64 @__mux_get_global_id(i32 2, ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK-DAG: [[OFX:%.*]] = call i64 @__mux_get_global_offset(i32 0, ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK-DAG: [[OFY:%.*]] = call i64 @__mux_get_global_offset(i32 1, ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK-DAG: [[OFZ:%.*]] = call i64 @__mux_get_global_offset(i32 2, ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK-DAG: [[SZX:%.*]] = call i64 @__mux_get_global_size(i32 0, ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK-DAG: [[SZY:%.*]] = call i64 @__mux_get_global_size(i32 1, ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK-DAG: [[TMP0:%.*]] = sub i64 [[IDZ]], [[OFZ]]
; CHECK-DAG: [[TMP1:%.*]] = mul i64 [[TMP0]], [[SZY]]
; CHECK-DAG: [[TMP2:%.*]] = sub i64 [[IDY]], [[OFY]]
; CHECK-DAG: [[TMP3:%.*]] = add i64 [[TMP1]], [[TMP2]]
; CHECK-DAG: [[TMP4:%.*]] = mul i64 [[TMP3]], [[SZX]]
; CHECK-DAG: [[TMP5:%.*]] = sub i64 [[IDX]], [[OFX]]
; CHECK-DAG: [[TMP6:%.*]] = add i64 [[TMP5]], [[TMP4]]
; CHECK-DAG: ret i64 [[TMP6]]

declare !mux_scheduled_fn !2 i64 @__mux_get_enqueued_local_size(i32)
; CHECK: define internal i64 @__mux_get_enqueued_local_size(i32 [[IDX:%.*]], ptr [[WIATTRS]] [[WI:%.*]], ptr [[WGATTRS]] [[WG:%.*]])
; CHECK: [[TMP0:%.*]] = call i64 @__mux_get_local_size(i32 [[IDX]], ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK: ret i64 [[TMP0]]

!0 = !{i32 0, i32 1}
!1 = !{i32 1, i32 2}
!2 = !{i32 -1, i32 1}
!3 = !{i32 0, i32 1}
