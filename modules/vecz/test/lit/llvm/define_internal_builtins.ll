; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k dummy -vecz-simd-width=4 -vecz-passes=define-builtins -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @dummy(i32 addrspace(2)* %in, i32 addrspace(1)* %out) {
  %b = bitcast i32 addrspace(2)* %in to <4 x i32> addrspace(2)*
  %v = call <4 x i32> @__vecz_b_masked_load4_Dv4_jPU3AS2Dv4_jDv4_b(<4 x i32> addrspace(2)* %b, <4 x i1> zeroinitializer)
  ret void
}

declare <4 x i32> @__vecz_b_masked_load4_Dv4_jPU3AS2Dv4_jDv4_b(<4 x i32> addrspace(2)*, <4 x i1>)
; CHECK-LABEL-GE15: define <4 x i32> @__vecz_b_masked_load4_Dv4_jPU3AS2Dv4_jDv4_b(ptr addrspace(2){{.*}}, <4 x i1>{{.*}}) {
; CHECK-LABEL-LT15: define <4 x i32> @__vecz_b_masked_load4_Dv4_jPU3AS2Dv4_jDv4_b(<4 x i32> addrspace(2)*{{.*}}, <4 x i1>{{.*}}) {
; CHECK-GE15:   %2 = call <4 x i32> @llvm.masked.load.v4i32.p2(ptr addrspace(2) %0, i32 4, <4 x i1> %1, <4 x i32> {{undef|poison}})
; CHECK-LT15:   %2 = call <4 x i32> @llvm.masked.load.v4i32.p2v4i32(<4 x i32> addrspace(2)* %0, i32 4, <4 x i1> %1, <4 x i32> undef)
; CHECK:   ret <4 x i32> %2
; CHECK: }
