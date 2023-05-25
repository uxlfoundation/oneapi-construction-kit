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

; RUN: muxc --device "%default_device" --passes optimal-builtin-replace,verify -S %s \
; RUN:   | FileCheck %s --check-prefix CHECK-NO-ANALYSIS
; RUN: muxc --device "%default_device" --passes "require<builtin-info>,optimal-builtin-replace,verify" -S %s \
; RUN:   | FileCheck %s --check-prefix CHECK-ANALYSIS

target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
target triple = "x86_64-unknown-unknown-elf"

; CHECK-NO-ANALYSIS: call i32 @_Z5isnanf(float %f)

; Don't check the whole sequence, but check we've inlined isnan.
; CHECK-ANALYSIS: [[T0:%.*]] = bitcast float %f to i32
; CHECK-ANALYSIS: [[T1:%.*]] = and i32 [[T0]], 2147483647

define spir_kernel void @do_isnan(float %f) {
  %v = call i32 @_Z5isnanf(float %f)
  ret void
}

declare i32 @_Z5isnanf(float)
