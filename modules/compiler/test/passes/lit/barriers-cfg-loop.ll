; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes barriers-pass,verify -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

; makes sure there's an unconditional branch between the subkernels

; CHECK: void @barrier_cfg_loop.mux-barrier-wrapper(
; CHECK: entry:
; CHECK: br label %sw.bb1

; The first subkernel branches unconditionally to the second
; CHECK: sw.bb1:
; CHECK: call {{.*}}i32 @barrier_cfg_loop.mux-barrier-region(
; CHECK-NOT: store
; CHECK: exitIR{{[0-9]*}}:
; CHECK: exitIR{{[0-9]*}}:
; CHECK: exitIR{{[0-9]*}}:
; CHECK  br label %sw.bb2

; The second subkernel branches conditionally to the exit or loops
; CHECK: sw.bb2:
; CHECK: call void @__mux_mem_barrier(i32 1, i32 272)
; CHECK: %[[ID:.+]] = call {{.*}}i32 @barrier_cfg_loop.mux-barrier-region.1(
; CHECK: store i32 %[[ID]], {{(ptr|i32\*)}} %next_barrier_id
; CHECK: exitIR{{[0-9]*}}:
; CHECK: exitIR{{[0-9]*}}:
; CHECK: exitIR{{[0-9]*}}:
; CHECK: %[[NEXT:.+]] = load i32, {{(ptr|i32\*)}} %next_barrier_id
; CHECK: %[[CMP:.+]] = icmp eq i32 %[[NEXT]], 0
; CHECK: br i1 %[[CMP]], label %kernel.exit, label %sw.bb2

; CHECK: kernel.exit:
; CHECK: ret void

define void @barrier_cfg_loop(i32 addrspace(1)* %d, i32 addrspace(1)* %a) #0 {
entry:
  %call = tail call i64 @_Z13get_global_idj(i32 0)
  br label %for.body

for.cond.cleanup:                                 ; preds = %for.body
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %call
  store i32 %add1, i32 addrspace(1)* %arrayidx2, align 4
  ret void

for.body:                                         ; preds = %for.body, %entry
  %i.08 = phi i64 [ 0, %entry ], [ %inc, %for.body ]
  %sum.07 = phi i32 [ 0, %entry ], [ %add1, %for.body ]
  %mul = shl i64 %i.08, 8
  %add = add i64 %mul, %call
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %a, i64 %add
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %add1 = add nsw i32 %0, %sum.07
  tail call void @__mux_work_group_barrier(i32 0, i32 1, i32 272)
  %inc = add nuw nsw i64 %i.08, 1
  %cmp.not = icmp eq i64 %inc, 10
  br i1 %cmp.not, label %for.cond.cleanup, label %for.body
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
