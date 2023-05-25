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

; RUN: muxc --device "%default_device" --passes run-vecz -S %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; The purpose of this test is to make sure we vectorize kernels whose input
; local size is not a power of 2 but that is still divisible by 2.
; FIXME: We are vectorizing by 8 and using 4 scalar iterations. We could
; instead vectorize by 4 without any scalar iterations. This would ideally be
; controlled by the target somehow.

; CHECK-LABEL: define spir_kernel void @__vecz_v8_foo(
define spir_kernel void @foo(i32 addrspace(1)* %in) #0 !reqd_work_group_size !0 {
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  ret void
}

; CHECK-LABEL: define spir_kernel void @__vecz_v8_bar(
define spir_kernel void @bar(i32 addrspace(1)* %in) #0 !reqd_work_group_size !1 {
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

attributes #0 = { "mux-kernel"="entry-point" "vecz-mode"="auto" }

!0 = !{ i32 12, i32 1, i32 1 }
!1 = !{ i32 14, i32 1, i32 1 }
