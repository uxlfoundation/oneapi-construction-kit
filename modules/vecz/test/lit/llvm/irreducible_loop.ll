; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k irreducible_loop -S < %s | %filecheck %t

; ModuleID = 'Unknown buffer'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @irreducible_loop(i32 addrspace(1)* %src, i32 addrspace(1)* %dst) #0 {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0) #2
  %arrayidx4 = getelementptr inbounds i32, i32 addrspace(1)* %dst, i64 %call
  %ld = load i32, i32 addrspace(1)* %arrayidx4, align 4
  %cmp = icmp sgt i32 %ld, -1
  br i1 %cmp, label %label, label %do.body

do.body:                                          ; preds = %entry, %label
  %id.0 = phi i64 [ %conv10, %label ], [ %call, %entry ]
  br label %label

label:                                            ; preds = %entry, %do.body
  %id.1 = phi i64 [ %id.0, %do.body ], [ %call, %entry ]
  %conv10 = add i64 %id.1, 1
  %cmp11 = icmp slt i64 %conv10, 16
  br i1 %cmp11, label %do.body, label %do.end

do.end:                                           ; preds = %label
  ret void
}

; Function Attrs: convergent nounwind readonly
declare spir_func i64 @_Z13get_global_idj(i32)

; CHECK: define spir_kernel void @__vecz_v4_irreducible_loop
; CHECK: entry:
; CHECK:   br label %irr.guard.outer

; CHECK: irr.guard.outer:                                  ; preds = %irr.guard.pure_exit, %entry
; CHECK:   br label %irr.guard

; LLVM 16 re-orders the Basic Blocks, without any change to the CFG.
; CHECK-LE15: irr.guard.pure_exit:                              ; preds = %irr.guard
; CHECK-LE15:   br i1 %{{.+}}, label %do.end, label %irr.guard.outer

; CHECK: do.end:                                           ; preds = %irr.guard.pure_exit
; CHECK:   ret void

; CHECK: irr.guard:                                        ; preds = %irr.guard, %irr.guard.outer
; CHECK:   br i1 %{{.+}}, label %irr.guard.pure_exit, label %irr.guard

; CHECK-GT15: irr.guard.pure_exit:                              ; preds = %irr.guard
; CHECK-GT15:   br i1 %{{.+}}, label %do.end, label %irr.guard.outer
