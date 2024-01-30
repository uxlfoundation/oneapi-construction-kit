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

; REQUIRES: codegen_x86
; RUN: muxc --device "%default_device" --passes link-builtins,verify -S %s | FileCheck %s

target triple = "spir32-unknown-unknown"
target datalayout = "e-p:32:32:32-m:e-i64:64-f80:128-n8:16:32:64-S128"

; We don't really care *what* the abs function looks like, but check it's been materialized
; CHECK: define {{.*}}spir_func{{.*}} i32 @_Z3absi(i32 {{.*%.+}}){{.*}} {
; CHECK:   ret i32 {{.*}}
; CHECK: }

declare spir_func i32 @_Z3absi(i32)

define spir_kernel void @foo(i32 addrspace(1)* %in) {
  ret void
}
