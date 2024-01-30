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

; RUN: muxc --passes 'add-kernel-wrapper<unpacked>,verify' < %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: define internal spir_kernel void @foo(ptr addrspace(1) noundef nonnull %x, ptr addrspace(1) %y)
define spir_kernel void @foo(ptr addrspace(1) noundef nonnull %x, ptr addrspace(1) %y) #0 {
  ret void
}

; CHECK: define internal spir_kernel void @empty_args()
define spir_kernel void @empty_args() #0 {
  ret void
}

; CHECK: define spir_kernel void @foo.mux-kernel-wrapper(
; CHECK-SAME: ptr noundef nonnull dereferenceable(16) %packed-args)
; Check that the 'noundef' and 'nonnull' attributes are transferred to the load
; of %x, but not %y
; CHECK: %x = load ptr addrspace(1), ptr {{.*}}, align 8,
; CHECK-SAME:   !nonnull [[EMPTY:\![0-9]+]], !noundef [[EMPTY]]
; CHECK: %y = load ptr addrspace(1), ptr {{.*}}, align 8{{$}}
; CHECK: call spir_kernel void @foo({{.*}})

; Check we don't add 'nonnull', 'noundef', or 'dereferenceable# attributes to
; this parameter as it may be null, or empty.
; CHECK: define spir_kernel void @empty_args.mux-kernel-wrapper(ptr %packed-args)
; CHECK: call spir_kernel void @empty_args()

attributes #0 = { "mux-kernel"="entry-point" }
