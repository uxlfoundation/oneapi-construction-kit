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

; RUN: muxc --passes compute-local-memory-usage,verify -S %s | FileCheck %s

@a = internal addrspace(3) global i16 poison, align 2
@b = internal addrspace(3) global [4 x float] poison, align 4

declare spir_func void @ext_fn()

declare spir_func void @leaf_fn()

define spir_func void @helper_fn() {
  %ld = load i16, i16 addrspace(3)* @a, align 2
  ret void
}

define spir_func void @other_helper_fn() {
  call spir_func void @leaf_fn()
  ret void
}

; CHECK: define spir_kernel void @kernel_bar() [[ATTRS_BAR:#[0-9]+]] {
define spir_kernel void @kernel_bar() #0 {
  call spir_func void @ext_fn()
  call spir_func void @other_helper_fn()
  ret void
}

; CHECK: define spir_kernel void @kernel_foo() [[ATTRS_FOO:#[0-9]+]] {
define spir_kernel void @kernel_foo() #0 {
  call spir_func void @ext_fn()
  call spir_func void @helper_fn()
  %ld = load [4 x float], [4 x float] addrspace(3)* @b
  ret void
}

; Check that we overwrite the existing attribute
; CHECK-DAG: attributes [[ATTRS_BAR]] = { "mux-kernel"="entry-point" "mux-local-mem-usage"="0" }
; CHECK-DAG: attributes [[ATTRS_FOO]] = { "mux-kernel"="entry-point" "mux-local-mem-usage"="18" }

attributes #0 = { "mux-kernel"="entry-point" "mux-local-mem-usage"="4" }
