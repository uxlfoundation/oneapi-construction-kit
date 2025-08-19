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

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

declare i64 @__core_get_global_id(i32)

define spir_kernel void @kernel_main(i32 addrspace(1)* %a, i32 addrspace(1)* %b) {
entry:
  %gid = call i64 @__core_get_global_id(i32 0)
  %agep = getelementptr i32, i32 addrspace(1)* %a, i64 %gid
  %bgep = getelementptr i32, i32 addrspace(1)* %b, i64 %gid
  %load = load i32, i32 addrspace(1)* %bgep
  store i32 %load, i32 addrspace(1)* %agep
  ret void
}
