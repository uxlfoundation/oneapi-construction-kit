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

; Let vecz pick the right vectorization factor for this kernel check that the
; verification pass correctly notes we've satisifed the required sub-group
; size.
; RUN: env muxc --device "%riscv_device" \
; RUN:   --passes run-vecz,verify-reqd-sub-group-satisfied < %s \
; RUN: | FileCheck %s

; CHECK-LABEL: define void @__vecz_v8_bar_sg8(ptr addrspace(1) %in, ptr addrspace(1) %out) #0 !intel_reqd_sub_group_size !0 !codeplay_ca_vecz.derived !{{[0-9]+}} {

define void @bar_sg8(ptr addrspace(1) %in, ptr addrspace(1) %out) #0 !intel_reqd_sub_group_size !0 {
  %id = call i64 @__mux_get_global_id(i32 0)
  %in.addr = getelementptr i32, ptr addrspace(1) %in, i64 %id
  %x = load i32, ptr addrspace(1) %in.addr
  %y = add i32 %x, 1
  %out.addr = getelementptr i32, ptr addrspace(1) %out, i64 %id
  store i32 %y, ptr addrspace(1) %out.addr
  ret void
}

declare i64 @__mux_get_global_id(i32)

attributes #0 = { "mux-kernel"="entry-point" }

!0 = !{i32 8}
