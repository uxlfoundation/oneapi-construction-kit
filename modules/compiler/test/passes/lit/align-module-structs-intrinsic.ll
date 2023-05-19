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

; RUN: %muxc --passes align-module-structs,verify -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"

; Note: this custom(!) datalayout has preferred ABI alignments:
;   i32 - 64 bits
;   i64 - 128 bits
; We use these to trigger the struct alignment pass
target datalayout = "e-p:64:64:64-m:e-i32:32-i64:32"

; This test simply checks that if ever need to remap an intrinsic - a rare
; occurrance - we correctly preserve the fact that the intrinsic is an
; intrinsic. This is actually done via an internal assertion, but this test
; case covers that.

; CHECK: declare ptr @llvm.preserve.struct.access.index.p0.p0(ptr, i32 immarg, i32 immarg)

%structTyA = type { i32, i64 }

declare i32* @llvm.preserve.struct.access.index.p0i32.p0s_structTyAs(%structTyA*, i32, i32)
