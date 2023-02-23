; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -S < %s | %filecheck %t

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @remove_intptr(i8 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %0 = ptrtoint i8 addrspace(1)* %in to i64
  %shl = shl nuw nsw i64 %call, 2
  %add = add i64 %shl, %0
  %1 = inttoptr i64 %add to i32 addrspace(1)*
  %2 = load i32, i32 addrspace(1)* %1, align 4
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %call
  store i32 %2, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; CHECK: spir_kernel void @__vecz_v4_remove_intptr
; CHECK-NOT: ptrtoint
; CHECK-NOT: inttoptr
; CHECK-GE15: %remove_intptr = getelementptr i8, ptr addrspace(1) %in
; CHECK-GE15: %[[LOAD:.+]] = load <4 x i32>, ptr addrspace(1) %remove_intptr, align 4
; CHECK-LT15: %remove_intptr = getelementptr i8, i8 addrspace(1)* %in
; CHECK-LT15: %[[BCAST:.+]] = bitcast i8 addrspace(1)* %remove_intptr to <4 x i32> addrspace(1)*
; CHECK-LT15: %[[LOAD:.+]] = load <4 x i32>, <4 x i32> addrspace(1)* %[[BCAST]], align 4
; CHECK: store <4 x i32> %[[LOAD]]
