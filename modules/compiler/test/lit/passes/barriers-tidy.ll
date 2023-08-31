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

; RUN: muxc --passes work-item-loops,verify -S %s | FileCheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

; there should only be a single int in the barrier
; CHECK: %tidy_barrier_live_mem_info = type { i32 }

; CHECK: i32 @tidy_barrier.mux-barrier-region.1

; makes sure the global id call got duplicated after the barrier
; CHECK: call {{.*}}i64 @__mux_get_global_id(i32 {{()?}}0)

define void @tidy_barrier(i32 addrspace(1)* %in, i32 addrspace(1)* %out) #0 {
entry:
  %call = tail call i64 @__mux_get_global_id(i32 0) #4
  %call1 = tail call i64 @__mux_get_global_id(i32 1) #4
  %0 = shl i64 %call1, 32
  %idxprom = ashr exact i64 %0, 32
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 %idxprom
  %1 = load i32, i32 addrspace(1)* %arrayidx, align 4
  tail call void @__mux_work_group_barrier(i32 0, i32 1, i32 272)
  %2 = shl i64 %call, 32
  %idxprom3 = ashr exact i64 %2, 32
  %arrayidx4 = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 %idxprom3
  %3 = load i32, i32 addrspace(1)* %arrayidx4, align 4
  %mul = mul nsw i32 %3, %1
  %arrayidx6 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %idxprom3
  store i32 %mul, i32 addrspace(1)* %arrayidx6, align 4
  ret void
}

declare void @__mux_work_group_barrier(i32, i32, i32)

declare i64 @__mux_get_global_id(i32)

declare i64 @__mux_get_local_size(i32)

declare i64 @__mux_get_group_id(i32)

declare i64 @__mux_get_local_id(i32)

declare i64 @__mux_get_global_offset(i32)

declare void @__mux_set_local_id(i32, i64)

attributes #0 = { "mux-kernel"="entry-point" }
