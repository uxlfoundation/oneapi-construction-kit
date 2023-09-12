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

; Try and forcibly vectorize this no-vecz kernel by 8 and check that the
; vectorizer does not run, since the required sub-group size is 1. Then check
; that the verification pass correctly picks up that we have satisfied the
; kernel's required sub-group size by way of not vectorizing.
; RUN: env CA_RISCV_VF=8 muxc --device "%riscv_device" \
; RUN:   --passes run-vecz,verify-reqd-sub-group-satisfied < %s \
; RUN: | FileCheck %s

; CHECK-NOT: __vecz_
define void @foo_sg1() #0 !intel_reqd_sub_group_size !2 {
  ret void
}

attributes #0 = { "mux-kernel"="entry-point" }

!2 = !{i32 1}
