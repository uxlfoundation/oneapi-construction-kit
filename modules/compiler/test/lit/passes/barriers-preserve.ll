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

; RUN: muxc --passes work-item-loops,verify -S %s  | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

; CHECK: define internal void @foo(i8 signext %x, ptr byval(i32) %p) [[ATTRS:#[0-9]+]] !test [[TEST:\![0-9]+]] {
define void @foo(i8 signext %x, ptr byval(i32) %p) #0 !test !0 {
  ret void
}

; Check we copy over all the parameter attributes, parameter names, and function metadata
; CHECK: define void @baz.mux-barrier-wrapper(i8 signext %x, ptr byval(i32) %p) [[WRAPPER_ATTRS:#[0-9]+]] !test [[TEST]] !codeplay_ca_wrapper [[WRAPPER_MD:\![0-9]+]] {

; Check we call the original kernel with the right parameter attributes
; CHECK: call void @foo(i8 signext {{%.*}}, ptr byval(i32) {{%.*}})
declare i64 @__mux_get_local_size(i32)

; CHECK-DAG: attributes [[ATTRS]] = { alwaysinline optsize "foo"="bar" "mux-base-fn-name"="baz" }
; CHECK-DAG: attributes [[WRAPPER_ATTRS]] = { nounwind optsize "foo"="bar" "mux-base-fn-name"="baz" "mux-kernel"="entry-point" }

; CHECK-DAG: [[TEST]] = !{!"dummy"}
; CHECK-DAG: [[WRAPPER_MD]] = !{[[MAIN_MD:\![0-9]+]], null}
; CHECK-DAG: [[MAIN_MD]] = !{i32 1, i32 0, i32 0, i32 0}

attributes #0 = { "mux-kernel"="entry-point" "mux-base-fn-name"="baz" optsize "foo"="bar" }

!0 = !{!"dummy"}
