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

; RUN: %muxc --passes replace-c11-atomic-funcs,verify -S %s  | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

declare spir_func float @_Z25atomic_fetch_add_explicitPU3AS1Vff(ptr addrspace(1), float)

define spir_kernel void @foo(ptr addrspace(1) %in) {
; CHECK: = atomicrmw fadd ptr addrspace(1) %in, float 1.000000e+00 monotonic, align 4
  %a = call spir_func float @_Z25atomic_fetch_add_explicitPU3AS1Vff(ptr addrspace(1) %in, float 1.0)
  ret void
}

!opencl.ocl.version = !{!0}

!0 = !{i32 3, i32 0}
