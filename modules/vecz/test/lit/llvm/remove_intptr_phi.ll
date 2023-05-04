; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -S < %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @remove_intptr(i8 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %0 = ptrtoint i8 addrspace(1)* %in to i64
  %shl = shl nuw nsw i64 %call, 2
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %shl
  br label %for.body

for.cond.cleanup:                                 ; preds = %for.body
  ret void

for.body:                                         ; preds = %for.body, %entry
  %x.07 = phi i32 [ 0, %entry ], [ %inc, %for.body ]
  %intin.06 = phi i64 [ %0, %entry ], [ %add, %for.body ]
  %add = add i64 %intin.06, 4
  %1 = inttoptr i64 %add to i32 addrspace(1)*
  %2 = load i32, i32 addrspace(1)* %1, align 4
  store i32 %2, i32 addrspace(1)* %arrayidx, align 4
  %inc = add nuw nsw i32 %x.07, 1
  %exitcond.not = icmp eq i32 %inc, 4
  br i1 %exitcond.not, label %for.cond.cleanup, label %for.body
}

declare spir_func i64 @_Z13get_global_idj(i32)

; CHECK: spir_kernel void @__vecz_v4_remove_intptr
; CHECK-NOT: ptrtoint
; CHECK-NOT: inttoptr
; CHECK: %[[RPHI:.+]] = phi ptr addrspace(1) [ %in, %entry ], [ %[[RGEP:.+]], %for.body ]
; CHECK: %[[RGEP]] = getelementptr i8, ptr addrspace(1) %[[RPHI]], i{{32|64}} 4
; CHECK: load i32, ptr addrspace(1) %[[RGEP]], align 4
