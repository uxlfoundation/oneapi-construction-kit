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

; RUN: %muxc --passes transfer-kernel-metadata,verify -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: declare spir_kernel void @foo() [[FOO_ATTRS:#[0-9]+]]
declare spir_kernel void @foo()

; CHECK: declare spir_kernel void @bar() [[BAR_ATTRS:#[0-9]+]]
declare spir_kernel void @bar()

; CHECK-DAG: attributes [[FOO_ATTRS]] = { "mux-kernel"="entry-point" "mux-orig-fn"="foo" }
; CHECK-DAG: attributes [[BAR_ATTRS]] = { "mux-kernel"="entry-point" "mux-orig-fn"="bar" }

!0 = !{i32 4, i32 5, i32 6}
