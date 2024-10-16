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

; RUN: muxc --passes replace-module-scope-vars,verify -S %s  | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

@local_val = external addrspace(3) global i32

; CHECK: %localVarTypes = type { i32 }

define spir_kernel void @constant_array_local() {
entry:
  %wg_val_17.i = insertvalue [2 x i64] [i64 ptrtoint (ptr addrspacecast (ptr addrspace(3) @local_val to ptr) to i64), i64 poison], i64 0, 1
; CHECK: getelementptr inbounds %localVarTypes, ptr %0, i32 0, i32 0
; CHECK: addrspacecast ptr %{{[0-9]+}} to ptr addrspace(3)
; CHECK: addrspacecast ptr addrspace(3) %{{[0-9]+}} to ptr
; CHECK: ptrtoint ptr %{{[0-9]+}} to i64
; CHECK: insertvalue [2 x i64] undef, i64 %{{[0-9]+}}, 0
; CHECK: insertvalue [2 x i64] %{{[0-9]+}}, i64 poison, 1
  ret void
}
