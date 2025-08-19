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

; RUN: muxc --passes add-sched-params,define-mux-builtins,verify -S %s  | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

declare !mux_scheduled_fn !1 i64 @__mux_get_local_id(i32)
; CHECK: define internal i64 @__mux_get_local_id(i32 [[IDX:%.*]], ptr [[WIATTRS:noalias nonnull align 8 dereferenceable\(40\)]] [[WI:%.*]], ptr [[WGATTRS:noalias nonnull align 8 dereferenceable\(104\)]] {{.*}})
; CHECK: [[TMP0:%.*]] = icmp ult i32 [[IDX]], 3
; CHECK: [[TMP1:%.*]] = select i1 [[TMP0]], i32 [[IDX]], i32 0
; CHECK: [[TMP2:%.*]] = getelementptr %MuxWorkItemInfo, ptr [[WI]], i32 0, i32 0, i32 [[TMP1]]
; CHECK: [[TMP3:%.*]] = load i64, ptr [[TMP2]], align 8
; CHECK: [[TMP4:%.*]] = select i1 [[TMP0]], i64 [[TMP3]], i64 0
; CHECK: ret i64 [[TMP4]]
declare !mux_scheduled_fn !2 void @__mux_set_local_id(i32, i64)
; CHECK: define internal void @__mux_set_local_id(i32 [[IDX:%.*]], i64 [[V:%.*]], ptr [[WIATTRS]] [[WI:%.*]], ptr [[WGATTRS]] {{.*}})
; CHECK: [[TMP0:%.*]] = getelementptr %MuxWorkItemInfo, ptr [[WI]], i32 0, i32 0, i32 [[IDX]]
; CHECK: store i64 [[V]], ptr [[TMP0]], align 8
; CHECK: ret void

; CHECK: define internal i32 @__mux_get_sub_group_id(
declare !mux_scheduled_fn !0 i32 @__mux_get_sub_group_id()
; CHECK: define internal void @__mux_set_sub_group_id(
declare !mux_scheduled_fn !1 void @__mux_set_sub_group_id(i32)

; CHECK: define internal i32 @__mux_get_num_sub_groups(
declare !mux_scheduled_fn !0 i32 @__mux_get_num_sub_groups()
; CHECK: define internal void @__mux_set_num_sub_groups(
declare !mux_scheduled_fn !1 void @__mux_set_num_sub_groups(i32)

; CHECK: define internal i32 @__mux_get_max_sub_group_size(
declare !mux_scheduled_fn !0 i32 @__mux_get_max_sub_group_size()
; CHECK: define internal void @__mux_set_max_sub_group_size(
declare !mux_scheduled_fn !1 void @__mux_set_max_sub_group_size(i32)

declare !mux_scheduled_fn !0 i64 @__mux_get_local_linear_id()
; CHECK: define internal i64 @__mux_get_local_linear_id(ptr [[WIATTRS]] [[WI:%.*]], ptr [[WGATTRS]] [[WG:%.*]])
; CHECK-DAG: [[IDX:%.*]] = call i64 @__mux_get_local_id(i32 0, ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK-DAG: [[IDY:%.*]] = call i64 @__mux_get_local_id(i32 1, ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK-DAG: [[IDZ:%.*]] = call i64 @__mux_get_local_id(i32 2, ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK-DAG: [[SZX:%.*]] = call i64 @__mux_get_local_size(i32 0, ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK-DAG: [[SZY:%.*]] = call i64 @__mux_get_local_size(i32 1, ptr [[WIATTRS]] [[WI]], ptr [[WGATTRS]] [[WG]])
; CHECK:     [[TMP0:%.*]] = mul i64 [[IDZ]], [[SZY]]
; CHECK:     [[TMP1:%.*]] = mul i64 [[TMP0]], [[SZX]]
; CHECK:     [[TMP2:%.*]] = mul i64 [[IDY]], [[SZX]]
; CHECK:     [[TMP3:%.*]] = add i64 [[TMP1]], [[TMP2]]
; CHECK:     [[TMP4:%.*]] = add i64 [[TMP3]], [[IDX]]
; CHECK:     ret i64 [[TMP4]]

!0 = !{i32 0, i32 -1}
!1 = !{i32 1, i32 -1}
!2 = !{i32 2, i32 -1}
