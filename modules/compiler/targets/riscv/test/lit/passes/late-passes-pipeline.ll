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

; REQUIRES: ca_llvm_options

; RUN: muxc --device "%riscv_device" --passes riscv-late-passes --debug-pass-manager %s 2>&1 \
; RUN:   | FileCheck %s

; Check only a selection of passes to make sure this is doing roughly the right
; thing without being too specific.

; CHECK: Running pass: compiler::utils::TransferKernelMetadataPass on [module]

; CHECK: Running pass: compiler::utils::ReplaceAddressSpaceQualifierFunctionsPass on add (1 instruction)
; CHECK: Running pass: riscv::IRToBuiltinReplacementPass on [module]

; CHECK: Running pass: compiler::utils::DefineMuxBuiltinsPass on [module]

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @add(ptr addrspace(1) %in, ptr addrspace(1) %out) {
  ret void
}
