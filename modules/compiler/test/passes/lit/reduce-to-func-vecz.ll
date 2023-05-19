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

; RUN: %muxc --passes "reduce-to-func<names=foo>,verify" -S %s  | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: define spir_kernel void @foo(
define spir_kernel void @foo() !codeplay_ca_vecz.base !0 !codeplay_ca_vecz.base !2 {
  ret void
}

; Check that we keep a vectorized form, based on a two-way link
; CHECK: define spir_kernel void @__vecz_nxv1_foo(
define spir_kernel void @__vecz_nxv1_foo() !codeplay_ca_vecz.derived !1 {
  ret void
}

; CHeck that we keep a vectorized form, even if the link is one way
; CHECK: define spir_kernel void @__vecz_nxv2_foo(
define spir_kernel void @__vecz_nxv2_foo() {
  ret void
}

; This isn't a kernel we want to keep
; CHECK-NOT: define spir_kernel void @bar(
define spir_kernel void @bar() !codeplay_ca_vecz.base !4 {
  ret void
}

; Since we don't want to keep @bar, don't keep a derived vectorized form either
; CHECK-NOT: define spir_kernel void @__vecz_nxv1_bar(
define spir_kernel void @__vecz_nxv1_bar() !codeplay_ca_vecz.derived !5 {
  ret void
}

!0 = !{!6, ptr @__vecz_nxv1_foo}
!1 = !{!6, ptr @foo}

!2 = !{!7, ptr @__vecz_nxv2_foo}
!3 = !{!7, ptr @foo}

!4 = !{!6, ptr @__vecz_nxv1_bar}
!5 = !{!6, ptr @bar}

!6 = !{i32 1, i32 1, i32 0, i32 0}
!7 = !{i32 2, i32 1, i32 0, i32 0}
