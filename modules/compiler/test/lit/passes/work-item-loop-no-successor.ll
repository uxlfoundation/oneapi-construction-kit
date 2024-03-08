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

; RUN: muxc --passes work-item-loops,verify < %s | FileCheck %s
target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

define internal spir_kernel void @foo(ptr %an_arg) #0 {
entry:
  %mux_call = call i64 @__mux_get_local_id(i32 0) #12
  br label %barrier

barrier:                                          ; preds = %entry
  call void @__mux_work_group_barrier(i32 1, i32 2, i32 912) #12
  tail call void @llvm.trap()
  unreachable
}

declare i64 @__mux_get_local_id(i32)

declare void @__mux_work_group_barrier(i32, i32, i32)

declare void @llvm.trap()

declare void @llvm.assume(i1)

attributes #0 = { "mux-kernel"="entry-point" }

; CHECK: exitIR12:                                         ; preds = %exitIR11
; CHECK:   call void @llvm.trap()
; CHECK:   unreachable
