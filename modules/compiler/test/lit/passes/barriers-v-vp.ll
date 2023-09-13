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

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

define internal void @foo() !codeplay_ca_vecz.base !2 !codeplay_ca_vecz.base !3 {
  ret void
}

; Check we've stripped this VP kernel of its 'entry point' status, as it hasn't
; been given work-item loops. Check this by checking there aren't any attributes.
; CHECK: define internal void @__vecz_v2_vp_foo() !codeplay_ca_vecz.derived {{\![0-9]+}} {
define void @__vecz_v2_vp_foo() #0 !codeplay_ca_vecz.derived !5 {
  ret void
}

define void @__vecz_v2_foo() #0 !codeplay_ca_vecz.derived !4 {
  ret void
}

; Check we only define one wrapper - the vector-predicated entry point should
; be ignored.
; CHECK-NOT: define void @__vecz_v2_vp_foo.mux-barrier-wrapper

; CHECK: define void @__vecz_v2_foo.mux-barrier-wrapper() [[ATTRS:#[0-9]+]]

; Potentially skip the vector block
; CHECK: br i1 {{.*}}, label %[[VEC_PH:.*]], label %[[VEC_EXIT:.*]]

; CHECK: [[VEC_PH:.*]]:
; CHECK: br label %[[VEC_LOOP:.*]]

; Loop calling the vector kernel
; CHECK: [[VEC_LOOP]]:
; CHECK: call void @__vecz_v2_foo()
; CHECK: br i1 {{.*}}, label %[[VEC_LOOP]], label %[[VEC_EXIT]]

; CHECK: [[VEC_EXIT]]:
; Potentially skip the VP block
; CHECK: br i1 {{.*}}, label %[[TAIL_BLOCK:.*]], label %[[TAIL_EXIT:.*]]

; Straight-line block calling the vector predicated kernel
; CHECK: [[TAIL_BLOCK]]:
; CHECK: call void @__vecz_v2_vp_foo()
; CHECK: br label %[[TAIL_EXIT]]

; CHECK: [[TAIL_EXIT]]:

; Check we only define one wrapper - the vector-predicated entry point should
; be ignored.
; CHECK-NOT: define void @__vecz_v2_vp_foo.mux-barrier-wrapper

attributes #0 = { "mux-kernel"="entry-point" }

!0 = !{i32 2, i32 0, i32 0, i32 0}
!1 = !{i32 2, i32 0, i32 0, i32 1}

!2 = !{!0, ptr @__vecz_v2_foo}
!3 = !{!1, ptr @__vecz_v2_vp_foo}

!4 = !{!0, ptr @foo}
!5 = !{!1, ptr @foo}
