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

; REQUIRES: ca_llvm_options
; RUN: muxc --passes "mux-base<late-builtins>,verify" --debug-pass-manager %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix LATE-BUILTINS
; RUN: muxc --passes "mux-base<prepare-wg-sched>,verify" --debug-pass-manager %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix PREPARE-WG-SCHED
; RUN: muxc --passes "mux-base<wg-sched>,verify" --debug-pass-manager %s 2>&1 \
; RUN:   | FileCheck %s --check-prefixes PREPARE-WG-SCHED,WG-SCHED
; RUN: muxc --passes "mux-base<pre-vecz>,verify" --debug-pass-manager %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix PRE-VECZ

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; PRE-VECZ: Running pass: compiler::utils::OptimalBuiltinReplacementPass
; The work-group collective pass is 3.0 only
; PRE-VECZ: {{(Running pass 'replace-wgc')?}}
; PRE-VECZ: Running pass: compiler::utils::PrepareBarriersPass

; LATE-BUILTINS: Running pass: compiler::utils::LinkBuiltinsPass
; LATE-BUILTINS: Running pass: compiler::utils::DefineMuxDmaPass
; LATE-BUILTINS: Running pass: compiler::utils::ReplaceMuxMathDeclsPass
; LATE-BUILTINS: Running pass: compiler::utils::OptimalBuiltinReplacementPass
; LATE-BUILTINS: Running pass: compiler::utils::ReduceToFunctionPass
; LATE-BUILTINS: Running pass: InternalizePass
; LATE-BUILTINS: Running pass: compiler::utils::FixupCallingConventionPass

; PREPARE-WG-SCHED: Running pass: compiler::utils::AddSchedulingParametersPass
; PREPARE-WG-SCHED: Running pass: compiler::utils::DefineMuxBuiltinsPass

; WG-SCHED: Running pass: compiler::utils::AddKernelWrapperPass

; Check we've changed the calling convention as the last act
; LATE-BUILTINS: define internal void @add

; WG-SCHED: define spir_kernel void @add{{.*}} {
; WG-SCHED: define spir_kernel void @use_bi{{.*}} !mux_scheduled_fn ![[METADATA:[0-9]+]] {

; WG-SCHED: ![[METADATA]] = !{i32 2, i32 3}

define spir_kernel void @add(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
  ret void
}

define spir_kernel void @use_bi(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
  %v = call i64 @__mux_get_local_id(i32 0)
  ret void
}

declare i64 @__mux_get_local_id(i32)
