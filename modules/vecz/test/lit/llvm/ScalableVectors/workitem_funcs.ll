; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; REQUIRES: llvm-13+
; RUN: %veczc -k store_ult -vecz-scalable -vecz-simd-width=4 -S < %s | %filecheck %s

; Check that we can scalably-vectorize a call to get_global_id by using the
; stepvector intrinsic

target triple = "spir64-unknown-unknown"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @store_ult(i32* %out, i64* %N) {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0) #2
  %0 = load i64, i64* %N, align 8
  %cmp = icmp ult i64 %call, %0
  %conv = zext i1 %cmp to i32
  %arrayidx = getelementptr inbounds i32, i32* %out, i64 %call
  store i32 %conv, i32* %arrayidx, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; CHECK: define spir_kernel void @__vecz_nxv4_store_ult
; CHECK:   [[step:%[0-9.a-z]+]] = call <vscale x 4 x i64> @llvm.experimental.stepvector.nxv4i64()
; CHECK:   %{{.*}} = add <vscale x 4 x i64> %{{.*}}, [[step]]
