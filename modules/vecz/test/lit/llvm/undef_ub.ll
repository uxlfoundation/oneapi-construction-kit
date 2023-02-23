; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -k test -w 4 -S < %s | %filecheck %s

; ModuleID = 'Unknown buffer'
source_filename = "Unknown buffer"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind readonly
declare spir_func i32 @_Z14get_local_sizej(i32) #2

; Function Attrs: convergent nounwind readonly
declare spir_func i32 @_Z12get_local_idj(i32) #2

; Function Attrs: convergent nounwind
define spir_kernel void @test() #0 {
entry:
  %call8 = call spir_func i32 @_Z12get_local_idj(i32 0) #3
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* undef, i32 %call8
  %0 = load i8, i8 addrspace(1)* %arrayidx, align 1
  %conv9 = uitofp i8 %0 to float
  %phitmp = fptoui float %conv9 to i8
  %arrayidx16 = getelementptr inbounds i8, i8 addrspace(1)* undef, i32 %call8
  store i8 %phitmp, i8 addrspace(1)* %arrayidx16, align 1
  ret void
}

; The "undefs" in the above IR should "optimize" to a trap call and an unreachable
; terminator instruction.
; CHECK: define spir_kernel void @__vecz_v4_test
; On LLVM 13+ there's no such trap: the UB is just that the function returns early.
; CHECK: ret void
