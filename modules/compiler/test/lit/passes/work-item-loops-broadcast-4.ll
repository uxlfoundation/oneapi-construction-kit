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

; RUN: muxc --passes work-item-loops,verify < %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

declare i64 @__mux_get_global_id(i32)

declare i32 @__mux_work_group_broadcast_i32(i32, i32, i64, i64, i64)

define spir_kernel void @broadcast_i32(ptr addrspace(1) %in, ptr addrspace(1) %out) #0 !reqd_work_group_size !0 !codeplay_ca_vecz.base !1 {
entry:
  %call = tail call i64 @__mux_get_global_id(i32 0)
  %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %in, i64 %call
  %0 = load i32, ptr addrspace(1) %arrayidx, align 4
  %call1 = tail call i32 @__mux_work_group_broadcast_i32(i32 0, i32 %0, i64 1, i64 0, i64 0)
  %arrayidx2 = getelementptr inbounds i32, ptr addrspace(1) %out, i64 %call
  store i32 %call1, ptr addrspace(1) %arrayidx2, align 4
  ret void
}

define spir_kernel void @__vecz_v4_broadcast_i32(ptr addrspace(1) %in, ptr addrspace(1) %out) #1 !reqd_work_group_size !0 !codeplay_ca_vecz.derived !3 {
entry:
  %call = tail call i64 @__mux_get_global_id(i32 0)
  %arrayidx = getelementptr i32, ptr addrspace(1) %in, i64 %call
  %0 = load <4 x i32>, ptr addrspace(1) %arrayidx, align 4
  %1 = extractelement <4 x i32> %0, i64 1
  %call1 = tail call i32 @__mux_work_group_broadcast_i32(i32 0, i32 %1, i64 1, i64 0, i64 0)
  %.splatinsert = insertelement <4 x i32> poison, i32 %call1, i64 0
  %.splat = shufflevector <4 x i32> %.splatinsert, <4 x i32> poison, <4 x i32> zeroinitializer
  %arrayidx2 = getelementptr i32, ptr addrspace(1) %out, i64 %call
  store <4 x i32> %.splat, ptr addrspace(1) %arrayidx2, align 4
  ret void
}

; CHECK: getelementptr inbounds %broadcast_i32_live_mem_info, ptr %live_variables_peel, i64 1

attributes #0 = { "mux-kernel"="entry-point" }
attributes #1 = { "mux-base-fn-name"="__vecz_v4_broadcast_i32" "mux-kernel"="entry-point" }

!0 = !{i32 3, i32 1, i32 1}
!1 = !{!2, ptr @__vecz_v4_broadcast_i32}
!2 = !{i32 4, i32 0, i32 0, i32 0}
!3 = !{!2, ptr @broadcast_i32}
