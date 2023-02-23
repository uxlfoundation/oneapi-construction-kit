; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -S < %s -vecz-auto | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @no_vecz1(i32 addrspace(1)* %out, i32 %n) {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %cmp = icmp eq i64 %call, 0
  br i1 %cmp, label %for.cond.preheader, label %if.end

for.cond.preheader:                               ; preds = %entry
  %cmp19 = icmp sgt i32 %n, 0
  %spec.select = select i1 %cmp19, i32 %n, i32 0
  store i32 %spec.select, i32 addrspace(1)* %out, align 4
  br label %if.end

if.end:                                           ; preds = %for.cond.preheader, %entry
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; CHECK-NOT: insertelement
; CHECK-NOT: shufflevector
; CHECK-NOT: extractelement
; CHECK-NOT: define void @__vecz_b_masked_store
