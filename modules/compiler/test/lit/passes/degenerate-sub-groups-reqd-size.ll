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
; RUN: muxc --passes degenerate-sub-groups,verify -S %s | FileCheck %s

; Check that the DegenerateSubGroupPass does not clone any kerenels with
; required sub-group sizes.

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

; CHECK-NOT: {{(work_group|foo)}}

define spir_func i32 @clone_this(i32 %x) {
entry:
  %call = call spir_func i32 @__mux_sub_group_reduce_add_i32(i32 %x)
  ret i32 %call
}

define spir_func i32 @shared(i32 %x) {
entry:
  %sqr = mul i32 %x, %x
  ret i32 %sqr
}

define spir_func i32 @sub_groups(i32 %x) #0 !intel_reqd_sub_group_size !1 {
entry:
  %call1 = call spir_func i32 @clone_this(i32 %x)
  %call2 = call spir_func i32 @shared(i32 %x)
  %add = add i32 %call1, %call2
  ret i32 %add
}

define spir_func i32 @no_sub_groups(i32 %x) #0 !intel_reqd_sub_group_size !1 {
entry:
  %call = call spir_func i32 @shared(i32 %x)
  ret i32 %call
}

declare spir_func i32 @__mux_sub_group_reduce_add_i32(i32)

!opencl.ocl.version = !{!0}

!0 = !{i32 3, i32 0}
!1 = !{i32 4}

attributes #0 = { "mux-kernel"="entry-point" }
