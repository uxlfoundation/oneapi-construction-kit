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

; RUN: muxc --passes replace-module-scope-vars,verify -S %s  | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: %localVarTypes = type { %triplet, [8 x i8], %triplet, %triplet, [16 x i8], [5 x %triplet], [8 x i8], i64, i20, [4 x i8], i64 }
%triplet = type { i64, i64, i64 }

@a = internal addrspace(3) global %triplet poison, align 16
@b = internal addrspace(3) global %triplet poison, align 16
@c = internal addrspace(3) global %triplet poison, align 8
@d = internal addrspace(3) global [5 x %triplet] poison, align 32
@e = internal addrspace(3) global i64 0, align 16
@f = internal addrspace(3) global i20 0, align 1
@g = internal addrspace(3) global i64 100

define spir_kernel void @foo() {
  ; CHECK: getelementptr {{.*}}, i32 0, i32 0
  %a1 = load i64, ptr addrspace(3) @a, align 8
  ; CHECK: getelementptr {{.*}}, i32 0, i32 0
  ; CHECK: getelementptr {{.*}}, i64 0, i32 1
  %a2p = getelementptr inbounds %triplet, ptr addrspace(3) @a, i64 0, i32 1
  %a2 = load i64, ptr addrspace(3) %a2p, align 8

  ; CHECK: getelementptr {{.*}}, i32 0, i32 2
  %b1 = load i64, ptr addrspace(3) @b, align 8
  ; CHECK: getelementptr {{.*}}, i32 0, i32 2
  ; CHECK: getelementptr {{.*}}, i64 0, i32 1
  %b2p = getelementptr inbounds %triplet, ptr addrspace(3) @b, i64 0, i32 1
  %b2 = load i64, ptr addrspace(3) %b2p, align 8

  ; CHECK: getelementptr {{.*}}, i32 0, i32 3
  %c1 = load i64, ptr addrspace(3) @c, align 8
  ; CHECK: getelementptr {{.*}}, i32 0, i32 3
  ; CHECK: getelementptr {{.*}}, i64 0, i32 1
  %c2p = getelementptr inbounds %triplet, ptr addrspace(3) @c, i64 0, i32 1
  %c2 = load i64, ptr addrspace(3) %c2p, align 8

  ; CHECK: getelementptr {{.*}}, i32 0, i32 5
  %d1 = load i64, ptr addrspace(3) @d, align 8
  ; CHECK: getelementptr {{.*}}, i32 0, i32 5
  ; CHECK: getelementptr {{.*}}, i64 0, i32 3, i32 1
  %d2p = getelementptr inbounds [5 x %triplet], ptr addrspace(3) @d, i64 0, i32 3, i32 1
  %d2 = load i64, ptr addrspace(3) %d2p, align 8

  ; CHECK: getelementptr {{.*}}, i32 0, i32 7
  %e = load i64, ptr addrspace(3) @e, align 8

  ; CHECK: getelementptr {{.*}}, i32 0, i32 8
  %f = load i20, ptr addrspace(3) @f

  ; CHECK: getelementptr {{.*}}, i32 0, i32 10
  %g = load i1, ptr addrspace(3) @g

  ret void
}
