; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; REQUIRES: llvm-13+
; RUN: %veczc -k mask_varying -vecz-scalable -vecz-simd-width=4 -vecz-choices=VectorPredication -S < %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

; A kernel which should produce a uniform masked vector load where the mask is
; a single varying splatted bit.
define spir_kernel void @mask_varying(<4 x i32>* %aptr, <4 x i32>* %zptr) {
entry:
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %mod_idx = urem i64 %idx, 2
  %arrayidxa = getelementptr inbounds <4 x i32>, <4 x i32>* %aptr, i64 %idx
  %ins = insertelement <4 x i1> undef, i1 true, i32 0
  %cmp = icmp slt i64 %idx, 64
  br i1 %cmp, label %if.then, label %if.end
if.then:
  %v = load <4 x i32>, <4 x i32>* %aptr
  %arrayidxz = getelementptr inbounds <4 x i32>, <4 x i32>* %zptr, i64 %idx
  store <4 x i32> %v, <4 x i32>* %arrayidxz, align 16
  br label %if.end
if.end:
  ret void
; CHECK: define spir_kernel void @__vecz_nxv4_vp_mask_varying
; CHECK: [[CMP:%.*]] = icmp slt <vscale x 4 x i64> %{{.*}},
; CHECK: [[INS:%.*]] = insertelement <vscale x 4 x i32> poison, i32 [[VL:%.*]], {{(i32|i64)}} 0
; CHECK: [[SPLAT:%.*]] = shufflevector <vscale x 4 x i32> [[INS]], <vscale x 4 x i32> poison, <vscale x 4 x i32> zeroinitializer
; CHECK: [[IDX:%.*]] = call <vscale x 4 x i32> @llvm.experimental.stepvector.nxv4i32()
; CHECK: [[MASK:%.*]] = icmp ult <vscale x 4 x i32> [[IDX]], [[SPLAT]]
; CHECK: [[INP:%.*]] = select <vscale x 4 x i1> [[MASK]], <vscale x 4 x i1> [[CMP]], <vscale x 4 x i1> zeroinitializer
; CHECK: [[RED:%.*]] = call i1 @llvm.vector.reduce.or.nxv4i1(<vscale x 4 x i1> [[INP]])
; CHECK: [[REINS:%.*]] = insertelement <4 x i1> poison, i1 [[RED]], {{(i32|i64)}} 0
; CHECK: [[RESPLAT:%.*]] = shufflevector <4 x i1> [[REINS]], <4 x i1> poison, <4 x i32> zeroinitializer
}

declare spir_func i64 @_Z13get_global_idj(i32)
declare <4 x i32> @__vecz_b_masked_load4_Dv4_jPDv4_jDv4_b(<4 x i32>*, <4 x i1>)
