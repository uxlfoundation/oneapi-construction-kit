; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -k split_branch -vecz-simd-width=4 -vecz-passes=uniform-reassoc -S < %s | %filecheck %s

; ModuleID = 'Unknown buffer'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @split_branch(i32 addrspace(1)* noalias %a, i32 addrspace(1)* noalias %b, i32 addrspace(1)* noalias %d) #0 {
entry:
  %x = call spir_func i64 @_Z13get_global_idj(i32 0) #2
  %y = call spir_func i64 @_Z13get_global_idj(i32 1) #2
  %a_gep = getelementptr inbounds i32, i32 addrspace(1)* %a, i64 %x
  %b_gep = getelementptr inbounds i32, i32 addrspace(1)* %b, i64 %y
  %varying = load i32, i32 addrspace(1)* %a_gep
  %uniform = load i32, i32 addrspace(1)* %b_gep
  %cmp_v = icmp sgt i32 %varying, 0
  %cmp_u = icmp sgt i32 %uniform, 0
  %or_vu = or i1 %cmp_v, %cmp_u
  br i1 %or_vu, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %inc = add i32 %uniform, 1
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  %result = phi i32 [ %inc, %if.then ], [ %varying, %entry ]
  %d_gep = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %x
  store i32 %result, i32 addrspace(1)* %d_gep
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; This test checks that a conditional branch based on an OR of both
; a uniform and a varying value gets split into two separate branches
; CHECK: define spir_kernel void @__vecz_v4_split_branch

; CHECK: %cmp_v = icmp sgt i32 %varying, 0
; CHECK: %cmp_u = icmp sgt i32 %uniform, 0

; ensure the original binary operator got deleted
; CHECK-NOT: or i1
; CHECK: br i1 %cmp_u, label %if.then, label %entry.cond_split

; CHECK: entry.cond_split:
; CHECK: br i1 %cmp_v, label %if.then, label %if.end

; CHECK: if.then:
; CHECK: %inc = add i32 %uniform, 1
; CHECK: br label %if.end

; CHECK: if.end:
; CHECK: %[[RESULT:.+]] = phi i32 [ %inc, %if.then ], [ %varying, %entry.cond_split ]
; CHECK: store i32 %[[RESULT]], ptr addrspace(1) %{{.+}}
