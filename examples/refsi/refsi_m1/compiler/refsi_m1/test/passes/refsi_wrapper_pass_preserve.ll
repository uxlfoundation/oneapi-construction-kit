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

; RUN: muxc --device "%riscv_device" %s --passes refsi-wrapper,verify -S | FileCheck %s

target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
target triple = "riscv64-unknown-unknown-elf"

; Check the original attributes and metadata are preserved, but that the kernel entry point has been taken
; CHECK: define internal void @add(ptr readonly %packed_args, ptr readonly %wg_info) [[ATTRS:#[0-9]+]] !test [[TEST:\![0-9]+]] {

; Check that we've copied over all the parameter attributes, names, metadata and attributes
; CHECK: define void @add.refsi-wrapper(i64 %instance, i64 %slice, ptr readonly %packed_args, ptr readonly %wg_info) [[WRAPPER_ATTRS:#[0-9]+]] !test [[TEST]] {
; Check we call the original kernel with the right parameter attributes
; CHECK: call void @add(ptr readonly %packed_args, ptr readonly {{%.*}}) [[ATTRS]]
define void @add(ptr readonly %packed_args, ptr readonly %wg_info) #0 !test !0 {
  ret void
}

; Check we've preserved the original attributes, but that neither are marked 'alwaysinline'
; CHECK-DAG: attributes [[ATTRS]] = { noinline optsize "test"="attr" }
; CHECK-DAG: attributes [[WRAPPER_ATTRS]] = { noinline nounwind optsize "mux-base-fn-name"="add" "mux-kernel"="entry-point" "test"="attr" }

; CHECK-DAG: [[TEST]] = !{!"dummy"}

attributes #0 = { optsize noinline "mux-kernel"="entry-point" "test"="attr" }

!0 = !{!"dummy"}
