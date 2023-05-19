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

; RUN: env CA_RISCV_VF=8 %muxc --device "%riscv_device" \
; RUN:   --passes "run-vecz,barriers-pass,add-sched-params" -S %s | %filecheck %s

target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
target triple = "riscv64-unknown-unknown-elf"

declare spir_func i64 @__mux_get_global_id(i32)

define spir_kernel void @foo(i32 addrspace(1)* %a, i32 addrspace(1)* %z) #0 {
entry:
  %call = tail call spir_func i64 @__mux_get_global_id(i32 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %a, i64 %call
  %x = load i32, i32 addrspace(1)* %arrayidx, align 4
  %add = add nsw i32 %x, 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %z, i64 %call
  store i32 %add, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}

; Check that we use sensible names when wrapping for scheduling parameters. We
; end up with a long name, but we can't clash with the vectorized sub-kernels'
; names.
; Ensure also that we preserve the original name for future passes.
; CHECK: define spir_kernel void @__vecz_v8_foo.mux-barrier-wrapper.mux-sched-wrapper({{.*}}) [[ATTRS:#[0-9]+]]
; CHECK:   call void @__vecz_v8_foo.mux-sched-wrapper(
; CHECK:   call void @foo.mux-sched-wrapper(

; CHECK: attributes [[ATTRS]] = { nounwind "mux-base-fn-name"="__vecz_v8_foo" "mux-kernel"="entry-point" }

attributes #0 = { "mux-kernel"="entry-point" }
