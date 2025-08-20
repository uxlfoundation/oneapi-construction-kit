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

; RUN: muxc --passes work-item-loops,verify < %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

; CHECK: define internal void @foo() [[OLD_ATTRS:#[0-9]+]] {
define void @foo() #0 {
  ret void
}

; CHECK: define internal void @nosg_main() [[OLD_ATTRS]] !codeplay_ca_vecz.derived {{\![0-9]+}} {
define void @nosg_main() #0 !codeplay_ca_vecz.derived !1 {
  ret void
}

; CHECK: define internal void @nosg_tail() [[OLD_NOSG_ATTRS:#[0-9]+]] !codeplay_ca_vecz.base {{\![0-9]+}} {
define void @nosg_tail() #0 !codeplay_ca_vecz.base !0 {
  ret void
}

; CHECK: define internal void @uses_sg_main() [[OLD_ATTRS]] !codeplay_ca_vecz.derived {{\![0-9]+}} {
define void @uses_sg_main() #0 !codeplay_ca_vecz.derived !4 {
  %x = call i32 @__mux_get_sub_group_local_id()
  ret void
}

; CHECK: define internal void @uses_sg_tail() [[OLD_ATTRS]] !codeplay_ca_vecz.base {{\![0-9]+}} {
define void @uses_sg_tail() #0 !codeplay_ca_vecz.base !3 {
  ret void
}

; CHECK: define internal void @reqd_sg_main() [[OLD_ATTRS]] !codeplay_ca_vecz.derived {{\![0-9]+}} !intel_reqd_sub_group_size {{\![0-9]+}} {
define void @reqd_sg_main() #0 !codeplay_ca_vecz.derived !6 !intel_reqd_sub_group_size !7 {
  ret void
}

; CHECK: define internal void @reqd_sg_tail() [[OLD_NOSG_ATTRS]] !codeplay_ca_vecz.base {{\![0-9]+}} !intel_reqd_sub_group_size {{\![0-9]+}} {
define void @reqd_sg_tail() #0 !codeplay_ca_vecz.base !5 !intel_reqd_sub_group_size !7 {
  ret void
}

; CHECK: define internal void @reqd_wg_main() [[OLD_ATTRS]] !codeplay_ca_vecz.derived {{\![0-9]+}} !reqd_work_group_size {{\![0-9]+}} {
define void @reqd_wg_main() #0 !codeplay_ca_vecz.derived !9 !reqd_work_group_size !10 {
  ret void
}

; CHECK: define internal void @reqd_wg_tail() [[OLD_NOSG_ATTRS]] !codeplay_ca_vecz.base {{\![0-9]+}} !reqd_work_group_size {{\![0-9]+}} {
define void @reqd_wg_tail() #0 !codeplay_ca_vecz.base !8 !reqd_work_group_size !10 {
  ret void
}

; CHECK: define internal void @reqd_wg_sg_main() [[OLD_ATTRS]] !codeplay_ca_vecz.derived {{\![0-9]+}} !reqd_work_group_size {{\![0-9]+}} {
define void @reqd_wg_sg_main() #0 !codeplay_ca_vecz.derived !12 !reqd_work_group_size !10 {
  %id = call i32 @__mux_get_sub_group_local_id()
  ret void
}

; CHECK: define internal void @reqd_wg_sg_tail() [[OLD_NOSG_ATTRS]] !codeplay_ca_vecz.base {{\![0-9]+}} !reqd_work_group_size {{\![0-9]+}} {
define void @reqd_wg_sg_tail() #0 !codeplay_ca_vecz.base !11 !reqd_work_group_size !10 {
  %id = call i32 @__mux_get_sub_group_local_id()
  ret void
}

declare i32 @__mux_get_sub_group_local_id()

; Check we've defined a wrapper for 'foo' (because it was marked an entry
; point).
; CHECK: define void @foo.mux-barrier-wrapper() {{#[0-9]+}}

; Check we've defined a wrapper for nosg's 'main' kernel (because it was marked
; an entry point).
; CHECK: define void @nosg_main.mux-barrier-wrapper() {{#[0-9]+}}

; Check we haven't defined another separate wrapper for nosg's 'tail' kernel.
; Even though it was marked an entry point, it's redundant as it's not used for
; a fallback sub-group kernel.
; CHECK-NOT: @nosg_tail.mux-barrier-wrapper()

; Check we've defined a wrapper for uses_sg's 'main' kernel (because it was
; marked an entry point).
; CHECK: define void @uses_sg_main.mux-barrier-wrapper() {{#[0-9]+}}

; Check we've defined a wrapper for uses_sg's 'tail' kernel (because it is
; required as a fallback kernel).
; CHECK: define void @uses_sg_tail.mux-barrier-wrapper() {{#[0-9]+}}

; Check we've defined a wrapper for reqd_sg's 'main' kernel.
; CHECK: define void @reqd_sg_main.mux-barrier-wrapper() {{#[0-9]+}}

; Check we haven't defined another separate wrapper for reqd_sg's 'tail' kernel.
; Even though it was marked an entry point, it's redundant as it has a required
; sub-group size that isn't 1. So no fallback is needed.
; CHECK-NOT: @reqd_sg_tail.mux-barrier-wrapper()

; Check we've defined a wrapper for reqd_wg's 'main' kernel
; CHECK: define void @reqd_wg_main.mux-barrier-wrapper() {{#[0-9]+}}

; Check we haven't defined another separate wrapper for reqd_wg's 'tail' kernel.
; Even though it was marked an entry point, it's redundant as it the main has a
; required work-group size and so covers the whole work-group without needing a
; tail.
; CHECK-NOT: @reqd_wg_tail.mux-barrier-wrapper()

; Check we've defined a wrapper for reqd_wg's 'main' kernel
; CHECK: define void @reqd_wg_sg_main.mux-barrier-wrapper() {{#[0-9]+}}

; Check we haven't defined another separate wrapper for reqd_wg's 'tail' kernel.
; Even though it was marked an entry point and uses sub-groups, it's redundant
; as it the main has a required work-group size and so covers the whole
; work-group without needing a tail.
; CHECK-NOT: @reqd_wg_sg_tail.mux-barrier-wrapper()

; Check we've stripped the old functions of their 'entry-point' status
; CHECK-DAG: attributes [[OLD_ATTRS]] = { alwaysinline convergent norecurse nounwind }
; CHECK-DAG: attributes [[OLD_NOSG_ATTRS]] = { convergent norecurse nounwind }

attributes #0 = { convergent norecurse nounwind "mux-kernel"="entry-point" }
attributes #1 = { convergent norecurse nounwind "mux-kernel"="entry-point" }

!0 = !{!2, ptr @nosg_main}
!1 = !{!2, ptr @nosg_tail}
!2 = !{i32 4, i32 0, i32 0, i32 0}
!3 = !{!2, ptr @uses_sg_main}
!4 = !{!2, ptr @uses_sg_tail}
!5 = !{!2, ptr @reqd_sg_main}
!6 = !{!2, ptr @reqd_sg_tail}
!7 = !{i32 4}
!8 = !{!2, ptr @reqd_wg_main}
!9 = !{!2, ptr @reqd_wg_tail}
!10 = !{i32 4, i32 1, i32 1}
!11 = !{!2, ptr @reqd_wg_sg_main}
!12 = !{!2, ptr @reqd_wg_sg_tail}
