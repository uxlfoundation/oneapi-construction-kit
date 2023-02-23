; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k atomic_rmw -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @atomic_cmpxchg_builtin(i32 addrspace(1)* %counter, i32 addrspace(1)* %out) {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %conv = trunc i64 %call to i32
  br label %do.body

do.body:                                          ; preds = %do.body, %entry
  %sub = add nsw i32 %conv, -1
  %0 = cmpxchg i32 addrspace(1)* %counter, i32 %sub, i32 %conv seq_cst acquire
  %1 = extractvalue { i32, i1 } %0, 0
  %sub2 = add nsw i32 %conv, -1
  %cmp = icmp eq i32 %1, %sub2
  br i1 %cmp, label %do.end, label %do.body

do.end:                                           ; preds = %do.body
  %idxprom = sext i32 %conv to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %idxprom
  store volatile i32 %1, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

define spir_kernel void @atomic_atomicrmw_builtin(i32 addrspace(1)* %counter, i32 addrspace(1)* %out) {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %conv = trunc i64 %call to i32
  br label %do.body

do.body:                                          ; preds = %do.body, %entry
  %sub = add nsw i32 %conv, -1
  %0 = atomicrmw nand i32 addrspace(1)* %counter, i32 %sub  acq_rel
  %sub2 = add nsw i32 %conv, -1
  %cmp = icmp eq i32 %0, %sub2
  br i1 %cmp, label %do.end, label %do.body

do.end:                                           ; preds = %do.body
  %idxprom = sext i32 %conv to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %idxprom
  store volatile i32 %0, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

define spir_kernel void @atomic_rmw(i32 addrspace(1)* %counter2, i32 addrspace(1)* %out) {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %conv = trunc i64 %call to i32
  %0 = atomicrmw add i32 addrspace(1)* %counter2, i32 1 seq_cst
  %idxprom = sext i32 %0 to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %idxprom
  store i32 %conv, i32 addrspace(1)* %arrayidx
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; CHECK: define spir_kernel void @__vecz_v4_atomic_rmw
; CHECK-GE15: atomicrmw add ptr addrspace(1) %counter2, i32 1 seq_cst
; CHECK-LT15: atomicrmw add i32 addrspace(1)* %counter2, i32 1 seq_cst
; CHECK-GE15: atomicrmw add ptr addrspace(1) %counter2, i32 1 seq_cst
; CHECK-LT15: atomicrmw add i32 addrspace(1)* %counter2, i32 1 seq_cst
; CHECK-GE15: atomicrmw add ptr addrspace(1) %counter2, i32 1 seq_cst
; CHECK-LT15: atomicrmw add i32 addrspace(1)* %counter2, i32 1 seq_cst
; CHECK-GE15: atomicrmw add ptr addrspace(1) %counter2, i32 1 seq_cst
; CHECK-LT15: atomicrmw add i32 addrspace(1)* %counter2, i32 1 seq_cst
; CHECK: ret void
