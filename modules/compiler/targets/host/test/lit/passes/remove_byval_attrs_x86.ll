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

; Run on any host device - it's the target we're compiling for that exhibits the bug
; RUN: muxc --device "%default_device" --passes "remove-byval-attrs" -S %s  | FileCheck %s

target triple = "x86_64-unknown-unknown-elf"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"

%struct.s = type { i32 }

define void @no_byval(ptr %a, ptr %b) {
  ret void
}

; CHECK: define void @byval_x(ptr %a, ptr %b, ptr %c) {
define void @byval_x(ptr %a, ptr byval(i32) %b, ptr byval(%struct.s) %c) {
  ret void
}

; CHECK: define void @byval_y(ptr %a) {
define void @byval_y(ptr byval(i32) %a) {
  ret void
}

define void @caller_byval() {
  %a = alloca i32
; CHECK: call void @byval_y(ptr %a)
  call void @byval_y(ptr byval(i32) %a)
  ret void
}
