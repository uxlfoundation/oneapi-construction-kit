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

; RUN: %muxc --passes bit-shift-fixup,verify -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK-LABEL: define spir_kernel void @foo(
; CHECK: %a = shl i8 %x, 4
; CHECK: %b = shl i8 %x, 1
; CHECK: [[TMP:%.*]] = and i8 %amt, 7
; CHECK: %c = shl i8 %x, [[TMP]]
; CHECK: %trunc = and i8 %amt, 7
; CHECK: %d = shl i8 %x, %trunc
define spir_kernel void @foo(i8 %x, i8 %amt) {
  %a = shl i8 %x, 4
  %b = shl i8 %x, 33
  %c = shl i8 %x, %amt
  %trunc = and i8 %amt, 7
  %d = shl i8 %x, %trunc
  ret void
}

; CHECK-LABEL: define spir_kernel void @foo_optnone(
; CHECK: %a = shl i8 %x, 4
; CHECK: %b = shl i8 %x, 1
; CHECK: [[TMP:%.*]] = and i8 %amt, 7
; CHECK: %c = shl i8 %x, [[TMP]]
; CHECK: %trunc = and i8 %amt, 7
; CHECK: %d = shl i8 %x, %trunc
define spir_kernel void @foo_optnone(i8 %x, i8 %amt) optnone noinline {
  %a = shl i8 %x, 4
  %b = shl i8 %x, 33
  %c = shl i8 %x, %amt
  %trunc = and i8 %amt, 7
  %d = shl i8 %x, %trunc
  ret void
}
