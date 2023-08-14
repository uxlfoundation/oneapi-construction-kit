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

; CHECK-LABEL: define spir_kernel void @__vecz_v4_foo(
define spir_kernel void @foo(i32 addrspace(1)* %in) #0 !reqd_work_group_size !0 {
  %gid = call i64 @__mux_get_global_id(i32 0)
  ret void
}

; CHECK-LABEL: define spir_kernel void @__vecz_v16_bar(
define spir_kernel void @bar(i32 addrspace(1)* %in) #0 !reqd_work_group_size !1 {
  ret void
}

declare i64 @__mux_get_global_id(i32)

attributes #0 = { "mux-kernel"="entry-point" "vecz-mode"="auto" }

!0 = !{ i32 5, i32 1, i32 1 }
!1 = !{ i32 17, i32 1, i32 1 }
