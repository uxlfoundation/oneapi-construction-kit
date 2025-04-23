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

; RUN: muxc --passes work-item-loops,verify -S %s  | FileCheck %s

; This test checks the validity of a set of main/tail loops when the
; barriers between vector and scalar kernels do not match up.

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

define spir_kernel void @foo(ptr addrspace(1) %a) !codeplay_ca_vecz.base !0 {
entry:
  store i1 false, ptr addrspace(1) %a, align 4
  call void @__mux_work_group_barrier(i32 0, i32 1, i32 272) #8
  %load = load i1, ptr addrspace(1) %a, align 4
  call void @__mux_work_group_barrier(i32 1, i32 1, i32 272) #8
  br i1 %load, label %bb.1, label %bb.2

bb.1:
  store i1 poison, ptr addrspace(1) poison
  call void @__mux_work_group_barrier(i32 2, i32 1, i32 272) #8
  br label %bb.3

bb.2:
  store i1 true, ptr addrspace(1) %a
  call void @__mux_work_group_barrier(i32 3, i32 1, i32 272) #8
  br label %bb.3

bb.3:
  call void @__mux_work_group_barrier(i32 4, i32 1, i32 272) #8
  ret void
}

define spir_kernel void @__vecz_v2_foo(ptr addrspace(1) %a) #0 !codeplay_ca_vecz.derived !2 {
entry:
  store i1 false, ptr addrspace(1) %a, align 4
  call void @__mux_work_group_barrier(i32 0, i32 1, i32 272)
  %load = load i1, ptr addrspace(1) %a, align 4
  call void @__mux_work_group_barrier(i32 1, i32 1, i32 272)
  %0 = xor i1 %load, true
  call void @llvm.assume(i1 %0)
  store i1 true, ptr addrspace(1) %a, align 1
  call void @__mux_work_group_barrier(i32 3, i32 1, i32 272)
  call void @__mux_work_group_barrier(i32 4, i32 1, i32 272)
  ret void
}

declare void @__mux_work_group_barrier(i32, i32, i32)

attributes #0 = { norecurse nounwind "mux-kernel"="entry-point" "mux-base-fn-name"="foo"}
attributes #1 = { nounwind "mux-barrier-schedule"="linear" }

; The block which has conditional undefined behavior in the scalar kernel.
;
; CHECK-LABEL: define internal spir_func i32 @foo.mux-barrier-region.6(ptr addrspace(1) %0, ptr %1)
; CHECK:       bb.1:
; CHECK-NEXT:    store i1 poison, ptr addrspace(1) poison, align 1
;
; The block which only follows after undefined behavior in the scalar kernel.
;
; CHECK-LABEL: define internal spir_func i32 @foo.mux-barrier-region.7(ptr addrspace(1) %0, ptr %1)
; CHECK-NEXT:  barrier2:
;
; Check that we do not call the unreachable region.
;
; CHECK-LABEL: define spir_kernel void @foo.mux-barrier-wrapper(ptr addrspace(1) %a)
; CHECK:         call spir_func i32 @foo.mux-barrier-region.6(
; CHECK-NOT:     call spir_func i32 @foo.mux-barrier-region.7(

; Vectorized by 2
!0 = !{!1, ptr @__vecz_v2_foo}
!1 = !{i32 2, i32 0, i32 0, i32 0}
!2 = !{!1, ptr @foo}
