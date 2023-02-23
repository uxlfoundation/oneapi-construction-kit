; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; REQUIRES: llvm-13+
; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k widen_vload -vecz-scalable -vecz-simd-width=4 -S < %s | %filecheck %t

target triple = "spir64-unknown-unknown"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @widen_vload(<4 x i32>* %aptr, <4 x i32>* %zptr) {
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %mod_idx = urem i64 %idx, 2
  %arrayidxa = getelementptr inbounds <4 x i32>, <4 x i32>* %aptr, i64 %mod_idx
  %v = load <4 x i32>, <4 x i32>* %arrayidxa, align 16
  %arrayidxz = getelementptr inbounds <4 x i32>, <4 x i32>* %zptr, i64 %idx
  store <4 x i32> %v, <4 x i32>* %arrayidxz, align 16
  ret void
; CHECK: define spir_kernel void @__vecz_nxv4_widen_vload(
; CHECK-GE15: %v4 = call <vscale x 16 x i32> @__vecz_b_gather_load16_u6nxv16ju10nxv16u3ptr(<vscale x 16 x ptr> %{{.*}})
; CHECK-LT15: %v4 = call <vscale x 16 x i32> @__vecz_b_gather_load16_u6nxv16ju7nxv16Pj(<vscale x 16 x i32*> %{{.*}})
}

declare spir_func i64 @_Z13get_global_idj(i32)
