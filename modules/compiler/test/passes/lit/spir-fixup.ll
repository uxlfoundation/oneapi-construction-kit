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

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %muxc --passes spir-fixup,verify -S %s | %filecheck %t

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

; Check that we change 'readnone' to 'readonly' on work-item functions
; CHECK: declare i64 @_Z13get_global_idj(i32) [[ATTRS:#[0-9]+]]
declare i64 @_Z13get_global_idj(i32) #0

define void @foo() #1 {
  ; CHECK: %x = call i64 @_Z13get_global_idj(i32 0) [[ATTRS]]
  %x = call i64 @_Z13get_global_idj(i32 0) #0
  ret void
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind }

; CHECK-GE16: attributes [[ATTRS]] = { nounwind memory(read) }
; CHECK-LT16: attributes [[ATTRS]] = { nounwind readonly }
