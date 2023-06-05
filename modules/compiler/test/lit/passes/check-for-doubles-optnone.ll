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

; RUN: muxc --passes "require<device-info>,check-doubles" -S %s 2>&1
; RUN: not muxc --passes check-doubles -S %s 2>&1 | FileCheck %s --check-prefix ERROR

; ERROR: error: A double precision floating point number was generated, but cl_khr_fp64 is not supported on this target.

; Check that optnone functions aren't skipped
define void @foo() optnone noinline {
  %a = fadd double 0.0, 1.0
  ret void

}
