; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes replace-wgc,prepare-barriers,barriers-pass,verify -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

; makes sure the accumulator initialization is not inside a work-item loop

; CHECK: i32 @reduction.mux-barrier-region.1(
; CHECK: store i32 0, {{(i32 addrspace\(3\)\*)|(ptr addrspace\(3\))}} @_Z21work_group_reduce_addi.accumulator

; CHECK: void @reduction.mux-barrier-wrapper(
; CHECK-LABEL: sw.bb2:
; CHECK: call void @__mux_mem_barrier(i32 2, i32 272)
; CHECK: call i32 @reduction.mux-barrier-region.1(

; CHECK: br label %sw.bb3

declare spir_func i64 @_Z13get_global_idj(i32 %x)
declare spir_func i32 @_Z21work_group_reduce_addi(i32 %x)

define internal void @reduction(i32 addrspace(1)* %d, i32 addrspace(1)* %a) #0 !reqd_work_group_size !0 {
entry:
  %call = tail call i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %a, i64 %call
  %ld = load i32, i32 addrspace(1)* %arrayidx, align 4
  %reduce = call i32 @_Z21work_group_reduce_addi(i32 %ld)
  store i32 %reduce, i32 addrspace(1)* %d, align 4
  ret void
}

attributes #0 = { "mux-kernel"="entry-point" }

!0 = !{i32 64, i32 64, i32 64}
