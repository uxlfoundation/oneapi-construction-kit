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

; RUN: muxc --passes replace-module-scope-vars  -S %s | FileCheck %s

; It checks that a comparison using a global doesn't crash the
; ReplaceLocalModuleScopeVariablesPass

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

@testKernel.lushort = internal addrspace(3) global i16 undef, align 2

declare i64 @__mux_get_global_id(i32)

; Function Attrs: norecurse nounwind
; CHECK: void @testKernel(ptr addrspace(1) nocapture writeonly align 4 %results, ptr [[STRUCT:%.*]])
define spir_kernel void @testKernel(ptr addrspace(1) nocapture writeonly align 4 %results) {
entry:
; CHECK: [[PTR0:%.*]] = getelementptr inbounds %localVarTypes, ptr [[STRUCT]], i32 0, i32 0
; CHECK: [[TMP0:%.*]] = addrspacecast ptr [[PTR0]] to ptr addrspace(3)
; CHECK: %cmp1 = icmp ne ptr addrspace(3) [[TMP0]], null
  %cmp1 = icmp ne ptr addrspace(3) @testKernel.lushort, null
; CHECK: [[PTR1:%.*]] = getelementptr inbounds %localVarTypes, ptr [[STRUCT]], i32 0, i32 0
; CHECK: [[TMP1:%.*]] = addrspacecast ptr [[PTR1]] to ptr addrspace(3)
; CHECK: %cmp2 = icmp ne ptr addrspace(3) [[TMP1]], [[TMP1]]
  %cmp2 = icmp ne ptr addrspace(3) @testKernel.lushort, @testKernel.lushort
  ret void
}
