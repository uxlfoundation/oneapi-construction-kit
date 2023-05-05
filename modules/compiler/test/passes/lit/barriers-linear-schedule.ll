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

; RUN: %muxc --passes barriers-pass,verify -S %s  | %filecheck %s

; This test checks the validity of a set of main/tail loops in conjunction with
; the 'linear' work-item order. The sub-group IVs must be carefully managed
; across all loops. See CA-4688.

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

define internal void @foo(ptr addrspace(1) %a) !codeplay_ca_vecz.base !2 {
entry:
  store i32 0, ptr addrspace(1) %a, align 4
  call void @__mux_work_group_barrier(i32 0, i32 1, i32 272) #8
  store i32 1, ptr addrspace(1) %a, align 4
  ret void
}

define void @__vecz_v2_foo(ptr addrspace(1) %a) #0 !codeplay_ca_vecz.derived !3 {
entry:
  store <2 x i32> <i32 0, i32 1>, ptr addrspace(1) %a, align 4
  call void @__mux_work_group_barrier(i32 0, i32 1, i32 272) #1
  store <2 x i32> <i32 2, i32 3>, ptr addrspace(1) %a, align 4
  ret void
}

; CHECK: define internal i32 @__vecz_v2_foo.mux-barrier-region(
; CHECK-NEXT: entry:
; CHECK-NEXT:  store <2 x i32> <i32 0, i32 1>
; CHECK-NEXT:  ret i32 2

; CHECK: define internal i32 @__vecz_v2_foo.mux-barrier-region.1(
; CHECK-NEXT: barrier:
; CHECK-NEXT:  store <2 x i32> <i32 2, i32 3>
; CHECK-NEXT:  ret i32 0

; CHECK: define internal i32 @foo.mux-barrier-region(
; CHECK-NEXT: entry:
; CHECK-NEXT:  store i32 0,
; CHECK-NEXT:  ret i32 2

; CHECK: define internal i32 @foo.mux-barrier-region.2(
; CHECK-NEXT: barrier:
; CHECK-NEXT:  store i32 1,
; CHECK-NEXT:  ret i32 0

; CHECK: define void @foo.mux-barrier-wrapper(ptr addrspace(1) %a) #2 !codeplay_ca_vecz.derived !3 !codeplay_ca_wrapper !4 {

; Assume the fist two regions (which aren't 'linear') are okay
; CHECK: call i32 @__vecz_v2_foo.mux-barrier-region(
; CHECK: call i32 @foo.mux-barrier-region(

; CHECK: call void @__mux_mem_barrier(i32 1, i32 272)

; Outermost (z) loop
; CHECK: [[LOOPZ:.*]]:
; Start counting the subgroup ID at 0
; CHECK: %sg.z = phi i32 [ 0, {{%.*}} ], [ [[SGMERGE:%.*]], %[[V_LOOPYEXIT:.*]] ] 

; Middle (y) loop
; CHECK: [[LOOPY:.*]]:
; Propagate the subgroup ID
; CHECK: %sg.y = phi i32 [ %sg.z, %[[V_LOOPZ:.*]] ], [ [[SGMERGE]], %[[V_LOOPXEXIT:.*]] ] 
; Skip the vector loop if there aren't any iterations to do
; CHECK: br i1 {{.*}}, label %[[V_PH:.*]], label %[[V_EXIT:.*]]

; Vector pre-header, branches to the vector loop
; CHECK: [[V_PH]]:
; CHECK: br label %[[V_LOOPX:.*]]

; Innermost vector loop, calling the vector kernel
; CHECK: [[V_LOOPX]]:
; CHECK: %sg.x.main = phi i32 [ %sg.y, %[[V_PH]] ], [ %sg.x.main.inc, %[[V_LOOPX]] ]
; CHECK: call i32 @__vecz_v2_foo.mux-barrier-region.1(
; CHECK: %sg.x.main.inc = add i32 %sg.x.main, 1
; CHECK: br i1 {{.*}}, label %[[V_LOOPX]], label %[[V_EXIT]]

; We need to merge the subgroup IV here, in case we skipped the vector loop
; CHECK: [[V_EXIT]]:
; CHECK: %sg.merge = phi i32 [ %sg.y, %[[LOOPY]] ], [ %sg.x.main.inc, %[[V_LOOPX]] ]
; Branch to either the scalar loop or scalar exit
; CHECK: br i1 {{.*}}, label %[[S_PH:.*]], label %[[S_EXIT:.*]]

; Scalar pre-header, branches to the scalar loop
; CHECK: [[S_PH]]:
; CHECK: br label %[[S_LOOPX:.*]]

; Innermost scalar loop, calling the scalar kernel
; CHECK: [[S_LOOPX]]:
; CHECK: %sg.x.tail = phi i32 [ %sg.merge, %[[S_PH]] ], [ %sg.x.tail.inc, %[[S_LOOPX]] ]
; CHECK: call i32 @foo.mux-barrier-region.2(
; CHECK: %sg.x.tail.inc = add i32 %sg.x.tail, 1
; CHECK: br i1 {{.*}}, label %[[S_LOOPX]], label %[[S_EXIT]]

; We need to merge the subgroup IV again here, in case we skipped the scalar loop
; CHECK: [[S_EXIT]]:
; CHECK: %sg.main.tail.merge = phi i32 [ %sg.x.tail.inc, %[[S_LOOPX]] ], [ %sg.merge, %[[V_EXIT]] ]
; CHECK: br i1 {{.*}}, label %[[LOOPY]], label %[[EXIT_Y:.*]]

; CHECK: [[EXIT_Y]]:
; CHECK: br i1 {{.*}}, label %[[LOOPZ]], label %[[EXIT_Z:.*]]

; CHECK: [[EXIT_Z]]:
; CHECK: br label %[[EXIT:.*]]

; CHECK: [[EXIT]]:
; CHECK: ret void


declare void @__mux_work_group_barrier(i32, i32, i32)

attributes #0 = { norecurse nounwind "mux-kernel"="entry-point" "mux-base-fn-name"="foo"}
attributes #1 = { nounwind "mux-barrier-schedule"="linear" }

; Vectorized by 2
!1 = !{i32 2, i32 0, i32 0, i32 0}
!2 = !{!1, ptr @__vecz_v2_foo}
!3 = !{!1, ptr @foo}
