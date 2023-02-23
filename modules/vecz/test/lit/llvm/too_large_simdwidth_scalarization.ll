; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; REQUIRES: linux
; RUN: %veczc -k add -vecz-simd-width=4 -S < %s | %filecheck %s

; ModuleID = 'kernel.opencl'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

; Function Attrs: nounwind
define spir_kernel void @add(<128 x i32>* %in1, <128 x i32>* %in2, <128 x i32>* %out) {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %in1p = getelementptr inbounds <128 x i32>, <128 x i32>* %in1, i64 %call
  %in1v = load <128 x i32>, <128 x i32>* %in1p, align 4
  %in2p = getelementptr inbounds <128 x i32>, <128 x i32>* %in2, i64 %call
  %in2v = load <128 x i32>, <128 x i32>* %in2p, align 4
  %add = add nsw <128 x i32> %in1v, %in2v
  %outp = getelementptr inbounds <128 x i32>, <128 x i32>* %out, i64 %call
  store <128 x i32> %add, <128 x i32>* %outp, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32) #2

; We do not expect this test to succeed
; XFAIL: *
