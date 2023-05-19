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

; RUN: %not %muxc --passes encode-kernel-metadata,verify -S %s 2>&1 | %filecheck %s --check-prefix INVALID_NAME
; RUN: %muxc --passes "encode-kernel-metadata<name=foo>,verify" -S %s | %filecheck %s --check-prefixes CHECK,DEFAULT

; RUN: %not %muxc --passes "encode-kernel-metadata<wibble>,verify" -S %s 2>&1 | %filecheck %s --check-prefix INVALID_OPTION

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; INVALID_NAME: EncodeKernelMetadataPass must be provided a 'name'
; INVALID_OPTION: invalid EncodeKernelMetadataPass parameter 'wibble'

; CHECK: declare spir_kernel void @foo() [[ATTRS:#[0-9]+]]
declare spir_kernel void @foo()

; CHECK-NOT: declare spir_kernel void @bar() {{#[0-9]+}}
declare spir_kernel void @bar()

; DEFAULT-DAG: attributes [[ATTRS]] = { "mux-kernel"="entry-point" "mux-orig-fn"="foo" }
