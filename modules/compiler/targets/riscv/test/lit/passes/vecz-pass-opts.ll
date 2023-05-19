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

; RUN: env CA_RISCV_VF=8 %muxc --device "%riscv_device" --passes "print<vecz-pass-opts>" -S %s 2>&1 \
; RUN:   | %filecheck %s --check-prefixes CHECK,CHECK-8
; RUN: env CA_RISCV_VF=1,S %muxc --device "%riscv_device" --passes "print<vecz-pass-opts>" -S %s 2>&1 \
; RUN:   | %filecheck %s --check-prefixes CHECK,CHECK-1S
; RUN: env CA_RISCV_VF=1,S,VP %muxc --device "%riscv_device" --passes "print<vecz-pass-opts>" -S %s 2>&1 \
; RUN:   | %filecheck %s --check-prefixes CHECK,CHECK-1S,CHECK-1S-VP
; RUN: env CA_RISCV_VF=1,S,VVP %muxc --device "%riscv_device" --passes "print<vecz-pass-opts>" -S %s 2>&1 \
; RUN:   | %filecheck %s --check-prefixes CHECK,CHECK-1S,CHECK-1S-VVP

target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
target triple = "riscv64-unknown-unknown-elf"

; CHECK: Function 'foo' will be vectorized {
; CHECK-8:  VF = 8
; CHECK-1S: VF = vscale x 1
; CHECK-SAME: vec-dim = 0, choices = [
; CHECK-NEXT:       DivisionExceptions
; CHECK-1S-VP-SAME: ,VectorPredication

; CHECK-1S-VVP:      VF = vscale x 1, vec-dim = 0, choices = [
; CHECK-1S-VVP-NEXT:   DivisionExceptions,VectorPredication

define spir_kernel void @foo(i32 addrspace(1)* %a, i32 addrspace(1)* %z) #0 {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %a, i64 %call
  %x = load i32, i32 addrspace(1)* %arrayidx, align 4
  %add = add nsw i32 %x, 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %z, i64 %call
  store i32 %add, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}

; optnone means don't optimize!
; CHECK: Function 'bar' will not be vectorized
define spir_kernel void @bar(i32 addrspace(1)* %a, i32 addrspace(1)* %z) #1 {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %a, i64 %call
  %x = load i32, i32 addrspace(1)* %arrayidx, align 4
  %add = add nsw i32 %x, 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %z, i64 %call
  store i32 %add, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

attributes #0 = { "mux-kernel"="entry-point" }
attributes #1 = { optnone noinline "mux-kernel"="entry-point" }
