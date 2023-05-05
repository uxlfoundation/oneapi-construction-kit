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

; RUN: %muxc %s --passes add-vectorize-metadata,verify -S | %filecheck %s

target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
target triple = "riscv64-unknown-unknown-elf"

; CHECK: @notes_global = constant [{{[0-9]+}} x i8] c"{{.*}}", section "notes", align 1

; Function Attrs: nounwind
define void @add() #5 !codeplay_ca_kernel !0 !codeplay_ca_wrapper !2 {
  ret void
}

!0 = !{i32 1}
; Fields for vectorization data are width, isScalable, SimdDimIdx, isVP
; Main vectorization of 1,S
!1 = !{i32 1, i32 1, i32 0, i32 0}
!2 = !{!1, !3}
; Tail is scalar
!3 = !{i32 1, i32 0, i32 0, i32 0}
