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

; RUN: muxc --device "%default_device" --passes "add-fp-control<ftz>,verify" -S %s | FileCheck %s

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-unknown-elf"

; CHECK: define internal spir_func void @add() [[ATTRS:#[0-9]+]] {

; CHECK: define spir_kernel void @add.host-fp-control() [[WRAPPER_ATTRS:#[0-9]+]] {

define spir_kernel void @add() #0 {
  ret void
}

; check that we haven't added 'alwaysinline' to this 'noinline' kernel
; CHECK: attributes [[ATTRS]] = { noinline }
; CHECK: attributes [[WRAPPER_ATTRS]] = { noinline nounwind "mux-base-fn-name"="add" "mux-kernel"="entry-point" }

attributes #0 = { noinline "mux-kernel"="entry-point" }
