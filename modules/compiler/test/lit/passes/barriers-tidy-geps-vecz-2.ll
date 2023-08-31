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

; there should only be a single int in the barrier
; CHECK: %tidy_barrier_live_mem_info = type { i32 }

; CHECK: i32 @tidy_barrier.mux-barrier-region.1

; makes sure the vector splat is duplicated after the barrier
; CHECK-DAG: insertelement <[[N:[0-9]+]] x i32>
; CHECK-DAG: shufflevector <[[N]] x i32> %{{.+}}, <[[N]] x i32> {{poison|undef}}, <[[N]] x i32> zeroinitializer

; makes sure the global id call got duplicated after the barrier
; CHECK-DAG: call {{.*}}i{{32|64}} @__mux_get_global_id(i{{32|64}}{{.*}} 0)

declare void @__mux_work_group_barrier(i32, i32, i32) #2

define void @tidy_barrier(ptr addrspace(1) %in, ptr addrspace(1) %out) #0 {
entry:
  %call = tail call i64 @__mux_get_global_id(i32 0) #4
  %call1 = tail call i64 @__mux_get_global_id(i32 1) #4
  %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %in, i64 %call1
  %0 = load i32, ptr addrspace(1) %arrayidx, align 4
  %.splatinsert = insertelement <16 x i32> poison, i32 %0, i64 0
  %.splat = shufflevector <16 x i32> %.splatinsert, <16 x i32> poison, <16 x i32> zeroinitializer
  tail call void @__mux_work_group_barrier(i32 0, i32 1, i32 272)
  %arrayidx4 = getelementptr inbounds i32, ptr addrspace(1) %in, i64 %call
  %1 = load <16 x i32>, ptr addrspace(1) %arrayidx4, align 4
  %mul1 = mul nsw <16 x i32> %1, %.splat
  %arrayidx6 = getelementptr inbounds i32, ptr addrspace(1) %out, i64 %call
  store <16 x i32> %mul1, ptr addrspace(1) %arrayidx6, align 4
  ret void
}

declare i64 @__mux_get_global_id(i32)

declare i64 @__mux_get_local_size(i32)

declare i64 @__mux_get_group_id(i32)

declare i64 @__mux_get_local_id(i32)

declare i64 @__mux_get_global_offset(i32)

declare void @__mux_set_local_id(i32, i64)

attributes #0 = { "mux-kernel"="entry-point" }
