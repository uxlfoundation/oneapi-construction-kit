; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes replace-barriers,verify -S %s | %filecheck %s

target datalayout = "e-p:32:32:32-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir-unknown-unknown"

define spir_kernel void @barriersKrnl() {
  ; ID, WorkGroup, SequentiallyConsistent|WorkGroupMemory|CrossWorkGroupMemory
  ; CHECK: call void @__mux_work_group_barrier(i32 0, i32 2, i32 784) [[NEW_ATTRS:#[0-9]+]]
  ; CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE
  call spir_func void @_Z7barrierj(i32 3) #0

  ; ID, WorkGroup, SequentiallyConsistent|WorkGroupMemory
  ; CHECK: call spir_func void @_Z18work_group_barrierj(i32 1) #0
  ; CLK_LOCAL_MEM_FENCE
  call spir_func void @_Z18work_group_barrierj(i32 1) #0

  ; ID, WorkGroup, SequentiallyConsistent|CrossWorkGroupMemory
  ; CHECK: call spir_func void @_Z18work_group_barrierj(i32 2) #0
  ; CLK_GLOBAL_MEM_FENCE
  call spir_func void @_Z18work_group_barrierj(i32 2) #0

  ; ID, WorkGroup, SequentiallyConsistent|WorkGroupMemory|CrossWorkGroupMemory
  ; CHECK: call spir_func void @_Z18work_group_barrierj(i32 3) #0
  ; CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE
  call spir_func void @_Z18work_group_barrierj(i32 3) #0

  ; ID, Device, SequentiallyConsistent|WorkGroupMemory|CrossWorkGroupMemory
  ; CHECK: call spir_func void @_Z18work_group_barrierjj(i32 3, i32 4) #0
  ; CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE, memory_scope_device
  call spir_func void @_Z18work_group_barrierjj(i32 3, i32 4) #0

  ; Equal to the above, but not everyone agrees on how to mangle 'memory_scope'
  ; CHECK: call spir_func void @_Z18work_group_barrierj12memory_scope(i32 3, i32 4) #0
  call spir_func void @_Z18work_group_barrierj12memory_scope(i32 3, i32 4) #0

  ; ID, SubGroup, SequentiallyConsistent|CrossWorkGroupMemory
  ; CHECK: call spir_func void @_Z17sub_group_barrierj(i32 2) #0
  ; CLK_GLOBAL_MEM_FENCE
  call spir_func void @_Z17sub_group_barrierj(i32 2) #0

  ; ID, WorkItem, SequentiallyConsistent|WorkGroupMemory|CrossWorkGroupMemory
  ; CHECK: call spir_func void @_Z17sub_group_barrierjj(i32 3, i32 1) #0
  ; CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE, memory_scope_work_item
  call spir_func void @_Z17sub_group_barrierjj(i32 3, i32 1) #0

  ; SubGroup, AcquireRelease|CrossWorkGroupMemory
  ; CHECK: call spir_func void @_Z22atomic_work_item_fencejjj(i32 2, i32 3, i32 2) #0
  ; CLK_GLOBAL_MEM_FENCE, memory_order_acq_rel, memory_scope_sub_group
  call spir_func void @_Z22atomic_work_item_fencejjj(i32 2, i32 3, i32 2) #0

  ; Equal to the above, but not everyone agrees on how to mangle 'memory_order'
  ; CHECK: call spir_func void @_Z22atomic_work_item_fencej12memory_orderj(i32 2, i32 3, i32 2) #0
  call spir_func void @_Z22atomic_work_item_fencej12memory_orderj(i32 2, i32 3, i32 2) #0

  ; Equal to the above, but not everyone agrees on how to mangle 'memory_scope'
  ; CHECK: call spir_func void @_Z22atomic_work_item_fencejj12memory_scope(i32 2, i32 3, i32 2) #0
  call spir_func void @_Z22atomic_work_item_fencejj12memory_scope(i32 2, i32 3, i32 2) #0

  ; Equal to the above, but not everyone agrees on how to mangle 'memory_order' or 'memory_scope'
  ; CHECK: call spir_func void @_Z22atomic_work_item_fencej12memory_order12memory_scope(i32 2, i32 3, i32 2) #0
  call spir_func void @_Z22atomic_work_item_fencej12memory_order12memory_scope(i32 2, i32 3, i32 2) #0

  ; WorkGroup, AcquireRelease|WorkGroupMemory
  ; CHECK: call void @__mux_mem_barrier(i32 2, i32 264) [[NEW_ATTRS]]
  ; CLK_LOCAL_MEM_FENCE
  call spir_func void @_Z9mem_fence(i32 1) #0

  ; WorkGroup, Acquire|CrossWorkGroupMemory
  ; CHECK: call void @__mux_mem_barrier(i32 2, i32 514) [[NEW_ATTRS]]
  ; CLK_GLOBAL_MEM_FENCE
  call spir_func void @_Z14read_mem_fence(i32 2) #0

  ; WorkGroup, Release|WorkGroupMemory|CrossWorkGroupMemory
  ; CHECK: call void @__mux_mem_barrier(i32 2, i32 772) [[NEW_ATTRS]]
  ; CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE
  call spir_func void @_Z15write_mem_fence(i32 3) #0

  ret void
}

declare spir_func void @_Z7barrierj(i32) #0

declare spir_func void @_Z18work_group_barrierj(i32) #0
declare spir_func void @_Z18work_group_barrierjj(i32, i32) #0
declare spir_func void @_Z18work_group_barrierj12memory_scope(i32, i32) #0

declare spir_func void @_Z17sub_group_barrierj(i32) #0
declare spir_func void @_Z17sub_group_barrierjj(i32, i32) #0

declare spir_func void @_Z9mem_fence(i32) #0
declare spir_func void @_Z14read_mem_fence(i32) #0
declare spir_func void @_Z15write_mem_fence(i32) #0

declare spir_func void @_Z22atomic_work_item_fencejjj(i32, i32, i32) #0
declare spir_func void @_Z22atomic_work_item_fencej12memory_orderj(i32, i32, i32) #0
declare spir_func void @_Z22atomic_work_item_fencejj12memory_scope(i32, i32, i32) #0
declare spir_func void @_Z22atomic_work_item_fencej12memory_order12memory_scope(i32, i32, i32) #0

; CHECK-DAG: declare void @__mux_mem_barrier(i32, i32) [[NEW_ATTRS]]

attributes #0 = { convergent }
; CHECK: attributes #0 = { convergent }
; CHECK: attributes [[NEW_ATTRS]] = { alwaysinline convergent noduplicate nomerge norecurse nounwind }

!opencl.ocl.version = !{!0}

!0 = !{i32 1, i32 2}
