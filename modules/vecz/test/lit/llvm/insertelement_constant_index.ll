; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k constant_index -vecz-simd-width=4 -vecz-choices=FullScalarization -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @constant_index(<4 x i32>* %in, <4 x i32>* %out) {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32>* %in, i64 %call
  %0 = load <4 x i32>, <4 x i32>* %arrayidx
  %arrayidx2 = getelementptr inbounds <4 x i32>, <4 x i32>* %out, i64 %call
  %vecins = insertelement <4 x i32> %0, i32 42, i32 2
  store <4 x i32> %vecins, <4 x i32>* %arrayidx2
  ret void
}

; CHECK: define spir_kernel void @__vecz_v4_constant_index

; We should only have 3 loads since one of the elements will be replaced
; CHECK-GE15: call <4 x i32> @__vecz_b_interleaved_load4_4_Dv4_ju3ptr
; CHECK-LT15: call <4 x i32> @__vecz_b_interleaved_load4_4_Dv4_jPj
; CHECK-GE15: call <4 x i32> @__vecz_b_interleaved_load4_4_Dv4_ju3ptr
; CHECK-LT15: call <4 x i32> @__vecz_b_interleaved_load4_4_Dv4_jPj
; CHECK-GE15: call <4 x i32> @__vecz_b_interleaved_load4_4_Dv4_ju3ptr
; CHECK-LT15: call <4 x i32> @__vecz_b_interleaved_load4_4_Dv4_jPj
; CHECK-NOT-GE15: call <4 x i32> @__vecz_b_interleaved_load4_4_Dv4_ju3ptr
; CHECK-NOT-LT15: call <4 x i32> @__vecz_b_interleaved_load4_4_Dv4_jPj

; We should have four stores, one of which would use the constant given
; CHECK: store <4 x i32>
; CHECK: store <4 x i32>
; CHECK: store <4 x i32>
; CHECK: store <4 x i32>
; CHECK: ret void
