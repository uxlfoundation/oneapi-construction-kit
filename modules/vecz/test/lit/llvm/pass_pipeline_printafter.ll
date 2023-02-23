; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; REQUIRES: llvm-12+
; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k foo -w 2 -vecz-passes scalarize,mask-memops,packetizer -print-after mask-memops -S < %s 2>&1 | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)

; CHECK: IR Dump After Simplify masked memory operations{{( on __vecz_v2_foo)?}}
; CHECK-NEXT-GE15: define spir_kernel void @__vecz_v2_foo(ptr addrspace(1) %out) #0 {
; CHECK-NEXT-LT15: define spir_kernel void @__vecz_v2_foo(i32 addrspace(1)* %out) #0 {
; CHECK-NEXT:   %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
; CHECK-NEXT-GE15:   %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %out, i64 %idx
; CHECK-NEXT-LT15:   %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %idx
; CHECK-NEXT-GE15:   store i32 0, ptr addrspace(1) %arrayidx, align 4
; CHECK-NEXT-LT15:   store i32 0, i32 addrspace(1)* %arrayidx, align 4
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

; CHECK-GE15: define spir_kernel void @__vecz_v2_foo(ptr addrspace(1) %out) {{.*}} {
; CHECK-LT15: define spir_kernel void @__vecz_v2_foo(i32 addrspace(1)* %out) {{.*}} {
; CHECK-NEXT:   %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
; CHECK-NEXT-GE15:   %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %out, i64 %idx
; CHECK-NEXT-LT15:   %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %idx
; CHECK-NEXT-LT15:   %1 = bitcast i32 addrspace(1)* %arrayidx to <2 x i32> addrspace(1)*
; CHECK-NEXT-GE15:   store <2 x i32> zeroinitializer, ptr addrspace(1) %arrayidx, align 4
; CHECK-NEXT-LT15:   store <2 x i32> zeroinitializer, <2 x i32> addrspace(1)* %1, align 4
; CHECK-NEXT:   ret void
; CHECK-NEXT: }

define spir_kernel void @foo(i32 addrspace(1)* %out) {
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %idx
  store i32 0, i32 addrspace(1)* %arrayidx, align 4
  ret void
}
