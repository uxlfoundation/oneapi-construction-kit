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

; RUN: muxc --passes define-mux-builtins,verify < %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

declare i64 @__mux_sub_group_shuffle_i64(i64 %val, i32 %lid)
; CHECK-LABEL: define i64 @__mux_sub_group_shuffle_i64(i64 %val, i32 %lid) {
; CHECK: ret i64 %val

declare half @__mux_sub_group_shuffle_xor_f16(half %val, i32 %xor_val)
; CHECK-LABEL: define half @__mux_sub_group_shuffle_xor_f16(half %val, i32 %xor_val) {
; CHECK: ret half %val

declare i8 @__mux_sub_group_shuffle_down_i8(i8 %curr, i8 %next, i32 %delta)
; CHECK-LABEL: define i8 @__mux_sub_group_shuffle_down_i8(i8 %curr, i8 %next, i32 %delta) {
; CHECK: %eqzero = icmp eq i32 %delta, 0
; CHECK: %sel = select i1 %eqzero, i8 %curr, i8 %next
; CHECK: ret i8 %sel

declare float @__mux_sub_group_shuffle_up_f32(float %prev, float %curr, i32 %delta)
; CHECK-LABEL: define float @__mux_sub_group_shuffle_up_f32(float %prev, float %curr, i32 %delta) {
; CHECK: %eqzero = icmp eq i32 %delta, 0
; CHECK: %sel = select i1 %eqzero, float %curr, float %prev
; CHECK: ret float %sel
