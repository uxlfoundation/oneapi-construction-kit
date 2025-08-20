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

; RUN: muxc --passes "reduce-to-func<names=A2B_KeepA:A2B_KeepB_Dep:B2A_KeepA_Dep:B2A_KeepB>,verify" -S %s  \
; RUN:   | FileCheck %s

; This test checks a series of one-way vectorization links, ensuring that we
; don't need two-directional ones to maintain depenent kernels during
; the reduce-to-func pass.

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: declare spir_kernel void @A2B_KeepA_Dep()
declare spir_kernel void @A2B_KeepA_Dep()

; CHECK: define spir_kernel void @A2B_KeepA()
define spir_kernel void @A2B_KeepA() !codeplay_ca_vecz.base !0 {
  ret void
}

; CHECK: declare spir_kernel void @A2B_KeepB_Dep()
declare spir_kernel void @A2B_KeepB_Dep()

; CHECK: define spir_kernel void @A2B_KeepB()
define spir_kernel void @A2B_KeepB() !codeplay_ca_vecz.base !1 {
  ret void
}

; CHECK: declare spir_kernel void @B2A_KeepA_Dep()
declare spir_kernel void @B2A_KeepA_Dep()

; CHECK: define spir_kernel void @B2A_KeepA()
define spir_kernel void @B2A_KeepA() !codeplay_ca_vecz.derived !2 {
  ret void
}

; CHECK: declare spir_kernel void @B2A_KeepB_Dep()
declare spir_kernel void @B2A_KeepB_Dep()

; CHECK: define spir_kernel void @B2A_KeepB()
define spir_kernel void @B2A_KeepB() !codeplay_ca_vecz.derived !3 {
  ret void
}

!0 = !{!4, ptr @A2B_KeepA_Dep}
!1 = !{!4, ptr @A2B_KeepB_Dep}
!2 = !{!4, ptr @B2A_KeepA_Dep}
!3 = !{!4, ptr @B2A_KeepB_Dep}

!4 = !{i32 4, i32 0, i32 0, i32 0}
