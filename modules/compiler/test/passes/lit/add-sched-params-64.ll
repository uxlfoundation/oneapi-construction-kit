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

; RUN: %muxc --passes add-sched-params,verify -S %s  | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

declare spir_func i64 @__mux_get_global_id(i32)

; Check that we preserve !test, but don't duplicate it

; CHECK: define spir_kernel void @foo.mux-sched-wrapper(ptr addrspace(1) %p, ptr noalias nonnull align 8 dereferenceable(40) %wi-info, ptr noalias nonnull align 8 dereferenceable(104) %wg-info)
; CHECK-SAME: [[ATTRS:#[0-9]+]] !test [[TEST_MD:\![0-9]+]] !mux_scheduled_fn [[SCHED_MD:\![0-9]+]] {
define spir_kernel void @foo(ptr addrspace(1) %p) #0 !test !0 {
  %call = tail call spir_func i64 @__mux_get_global_id(i32 0)
  ret void
}

; CHECK: attributes [[ATTRS]] = { "mux-base-fn-name"="foo" "test" "test-attr"="val" }

; CHECK: [[TEST_MD]] = !{i32 42}
; CHECK: [[SCHED_MD]] = !{i32 1, i32 2}

attributes #0 = { "test" "test-attr"="val" }

!0 = !{i32 42}
