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

; RUN: muxc --passes reduce-to-func,verify -S %s | FileCheck %s --check-prefix ALL
; RUN: muxc --passes "reduce-to-func<names=foo>",verify -S %s | FileCheck %s --check-prefix FOO
; RUN: muxc --passes "reduce-to-func<names=bar>",verify -S %s | FileCheck %s --check-prefix BAR
; RUN: muxc --passes "reduce-to-func<names=foo:bar>",verify -S %s | FileCheck %s --check-prefix ALL
; RUN: muxc --passes "reduce-to-func<names=baz>",verify -S %s | FileCheck %s --check-prefix BAZ

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; When no 'names' are given, the pass reduces to only those functions (and
; callees/references) with "entry-point" attributes.

; ALL: declare spir_kernel void @callee
; FOO: declare spir_kernel void @callee
; BAR-NOT: declare spir_kernel void @callee
; BAZ-NOT: declare spir_kernel void @callee
declare spir_kernel void @callee(i32 addrspace(1)* %in)

; ALL: define spir_kernel void @foo
; FOO: define spir_kernel void @foo
; BAR-NOT: define spir_kernel void @foo
; BAZ-NOT: define spir_kernel void @foo
define spir_kernel void @foo(i32 addrspace(1)* %in) #0 {
  call void @callee(i32 addrspace(1)* %in)
  ret void
}

; ALL: define spir_kernel void @bar
; FOO-NOT: define spir_kernel void @bar
; BAR: define spir_kernel void @bar
; BAZ-NOT: define spir_kernel void @bar
define spir_kernel void @bar(i32 addrspace(1)* %in) #0 {
  ret void
}

; ALL-NOT: define spir_kernel void @baz
; FOO-NOT: define spir_kernel void @baz
; BAR-NOT: define spir_kernel void @baz
; BAZ: define spir_kernel void @baz
define spir_kernel void @baz(i32 addrspace(1)* %in) {
  ret void
}

attributes #0 = { "mux-kernel"="entry-point" }
