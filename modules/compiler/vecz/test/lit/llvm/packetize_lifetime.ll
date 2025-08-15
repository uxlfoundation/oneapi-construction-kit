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

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: veczc -vecz-passes=packetizer -S < %s | FileCheck %t
; REQUIRES: llvm-22+

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare i64 @__mux_get_global_id(i32)

define spir_kernel void @__vecz_v64_fract_double3() {
entry:
  %iout.i = alloca <3 x double>, align 32
  %call.i = tail call i64 @__mux_get_global_id(i32 0)
  %cmp.i = icmp ult i64 %call.i, 0
  call void @llvm.lifetime.start.p0(ptr nonnull %iout.i)
  %.splatinsert = insertelement <4 x i1> zeroinitializer, i1 %cmp.i, i64 0
  call void null(<4 x double> zeroinitializer, ptr %iout.i, <4 x i1> %.splatinsert)
  call void @llvm.lifetime.end.p0(ptr nonnull %iout.i)
  ret void
}
; CHECK: spir_kernel void @__vecz_v4___vecz_v64_fract_double3
; CHECK: %iout.i1 = alloca <3 x double>, i64 4, align 3
; CHECK-GE22: call void @llvm.lifetime.start.p0(ptr nonnull %iout.i1)
; CHECK-GE22: call void @llvm.lifetime.end.p0(ptr nonnull %iout.i1)
; CHECK-LT22: call void @llvm.lifetime.start.p0(ptr nonnull %iout.i1)
; CHECK-LT22: call void @llvm.lifetime.end.p0(ptr nonnull %iout.i1)
; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)

