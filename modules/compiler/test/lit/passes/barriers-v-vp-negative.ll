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

define internal void @foo() !codeplay_ca_vecz.base !2 !codeplay_ca_vecz.base !3 {
  ret void
}

define void @__vecz_v2_foo() #0 !codeplay_ca_vecz.derived !4 {
  ret void
}

define void @__vecz_v2_vp_foo() #0 !codeplay_ca_vecz.derived !5 {
  ret void
}

; Check we define two wrappers for each entry point. The two kernels are
; related via different VFs, which we don't combine.
; CHECK: define void @__vecz_v2_foo.mux-barrier-wrapper
; CHECK: define void @__vecz_v2_vp_foo.mux-barrier-wrapper

attributes #0 = { "mux-kernel"="entry-point" }

!0 = !{i32 2, i32 0, i32 0, i32 0}
!1 = !{i32 4, i32 0, i32 0, i32 1}

!2 = !{!0, ptr @__vecz_v2_foo}
!3 = !{!1, ptr @__vecz_v2_vp_foo}

!4 = !{!0, ptr @foo}
!5 = !{!1, ptr @foo}
