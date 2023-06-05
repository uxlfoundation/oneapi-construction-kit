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

; RUN: muxc --passes barriers-pass,verify -S %s  | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

define internal void @foo() !codeplay_ca_vecz.base !1 {
  ret void
}

define void @__vecz_v4_foo() #0 !codeplay_ca_vecz.derived !4 {
  ret void
}

define internal void @bar() !codeplay_ca_vecz.base !2 !reqd_work_group_size !7 {
  ret void
}

define void @__vecz_v4_bar() #0 !codeplay_ca_vecz.derived !5 {
  ret void
}

define internal void @baz() !codeplay_ca_vecz.base !3 !reqd_work_group_size !8 {
  ret void
}

define void @__vecz_v4_baz() #0 !codeplay_ca_vecz.derived !6 {
  ret void
}

; CHECK: define void @__vecz_v4_foo.mux-barrier-wrapper(){{.*}}!codeplay_ca_wrapper [[FOO_WRAPPER_MD:\![0-9]+]] {

; CHECK: define void @__vecz_v4_bar.mux-barrier-wrapper(){{.*}}!codeplay_ca_wrapper [[SHARED_WRAPPER_MD:\![0-9]+]] {

; CHECK: define void @__vecz_v4_baz.mux-barrier-wrapper(){{.*}}!codeplay_ca_wrapper [[SHARED_WRAPPER_MD:\![0-9]+]] {

attributes #0 = { "mux-kernel"="entry-point" }

!0 = !{i32 4, i32 0, i32 0, i32 0}

!1 = !{!0, ptr @__vecz_v4_foo}
!2 = !{!0, ptr @__vecz_v4_bar}
!3 = !{!0, ptr @__vecz_v4_baz}

!4 = !{!0, ptr @foo}
!5 = !{!0, ptr @bar}
!6 = !{!0, ptr @baz}

; bar executes a WG size of 3 so will never execute its vectorized loop
!7 = !{i32 3, i32 1, i32 1}
; baz has a zero WG size? It doesn't make sense, but the compiler should be
; able to handle it.
!8 = !{i32 0, i32 1, i32 1}

; CHECK-DAG: [[MAIN_MD:\![0-9]+]] = !{i32 4, i32 0, i32 0, i32 0}
; CHECK-DAG: [[TAIL_MD:\![0-9]+]] = !{i32 1, i32 0, i32 0, i32 0}

; The wrapped @foo has a main and a tail
; CHECK-DAG: [[FOO_WRAPPER_MD]] = !{[[MAIN_MD]], [[TAIL_MD]]}
; The wrapped @bar expects only the tail as the main, since it'll never execute
; the vectorized loop.
; The wrapped @baz has neither main nor tail, so we encode a token unpredicated
; scalar main info, which happens to be the shared tail MD in this case.
; CHECK-DAG: [[SHARED_WRAPPER_MD]] = !{[[TAIL_MD]], null}
