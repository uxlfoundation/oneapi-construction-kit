; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes barriers-pass,verify -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

; there should only be a single int in the barrier
; CHECK: %tidy_barrier_live_mem_info = type { i32 }

; CHECK: i32 @tidy_barrier.mux-barrier-region

; makes sure the global id call got duplicated after the barrier
; CHECK: call {{.*}}i64 @_Z13get_global_idj(i32 {{()?}}0)

define void @tidy_barrier(i32 addrspace(1)* %in, i32 addrspace(1)* %out) #0 {
entry:
  %call = tail call i64 @_Z13get_global_idj(i32 0)
  %call1 = tail call i64 @_Z13get_global_idj(i32 1)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 %call1
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %add.ptr = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 %call
  %add.ptr2 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %call
  tail call void @__mux_work_group_barrier(i32 0, i32 1, i32 272)
  %1 = load i32, i32 addrspace(1)* %add.ptr, align 4
  %mul = mul nsw i32 %1, %0
  store i32 %mul, i32 addrspace(1)* %add.ptr2, align 4
  ret void
}

define internal i64 @_Z13get_global_idj(i32 %x) {
entry:
  %call = tail call i64 @__mux_get_global_id(i32 %x)
  ret i64 %call
}

declare void @__mux_work_group_barrier(i32, i32, i32)

declare i64 @__mux_get_global_id(i32)

declare i64 @__mux_get_local_size(i32)

declare i64 @__mux_get_group_id(i32)

declare i64 @__mux_get_local_id(i32)

declare i64 @__mux_get_global_offset(i32)

declare void @__mux_set_local_id(i32, i64)

attributes #0 = { "mux-kernel"="entry-point" }
