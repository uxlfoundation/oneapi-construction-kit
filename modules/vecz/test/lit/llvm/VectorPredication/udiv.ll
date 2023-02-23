; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; REQUIRES: llvm-13+

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k udiv -vecz-scalable -vecz-simd-width=2 -vecz-choices=VectorPredication -S < %s | %filecheck %t

target triple = "spir64-unknown-unknown"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @udiv(i32* %aptr, i32* %bptr, i32* %zptr) {
entry:
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidxa = getelementptr inbounds i32, i32* %aptr, i64 %idx
  %arrayidxb = getelementptr inbounds i32, i32* %bptr, i64 %idx
  %arrayidxz = getelementptr inbounds i32, i32* %zptr, i64 %idx
  %a = load i32, i32* %arrayidxa, align 4
  %b = load i32, i32* %arrayidxb, align 4
  %sum = udiv i32 %a, %b
  store i32 %sum, i32* %arrayidxz, align 4
  ret void
}

; CHECK: define spir_kernel void @__vecz_nxv2_vp_udiv(
; CHECK: [[LID:%.*]] = call i64 @__mux_get_local_id(i32 0)
; CHECK: [[LSIZE:%.*]] = call i64 @__mux_get_local_size(i32 0)
; CHECK: [[WREM:%.*]] = sub nuw nsw i64 [[LSIZE]], [[LID]]
; CHECK: [[T0:%.*]] = call i64 @llvm.vscale.i64()
; CHECK: [[T1:%.*]] = shl i64 [[T0]], 1
; CHECK: [[T2:%.*]] = call i64 @llvm.umin.i64(i64 [[WREM]], i64 [[T1]])
; CHECK: [[VL:%.*]] = trunc i64 [[T2]] to i32
; CHECK-GE15: [[LHS:%.*]] = call <vscale x 2 x i32> @llvm.vp.load.nxv2i32.p0(ptr {{%.*}}, [[TRUEMASK:<vscale x 2 x i1> shufflevector \(<vscale x 2 x i1> insertelement \(<vscale x 2 x i1> (undef|poison), i1 true, i32 0\), <vscale x 2 x i1> (undef|poison), <vscale x 2 x i32> zeroinitializer\)]], i32 [[VL]])
; CHECK-LT15: [[LHS:%.*]] = call <vscale x 2 x i32> @llvm.vp.load.nxv2i32.p0nxv2i32(<vscale x 2 x i32>* {{%.*}}, [[TRUEMASK:<vscale x 2 x i1> shufflevector \(<vscale x 2 x i1> insertelement \(<vscale x 2 x i1> (undef|poison), i1 true, i32 0\), <vscale x 2 x i1> (undef|poison), <vscale x 2 x i32> zeroinitializer\)]], i32 [[VL]])
; CHECK-GE15: [[RHS:%.*]] = call <vscale x 2 x i32> @llvm.vp.load.nxv2i32.p0(ptr {{%.*}}, [[TRUEMASK]], i32 [[VL]])
; CHECK-LT15: [[RHS:%.*]] = call <vscale x 2 x i32> @llvm.vp.load.nxv2i32.p0nxv2i32(<vscale x 2 x i32>* {{%.*}}, [[TRUEMASK]], i32 [[VL]])
; CHECK: [[ADD:%.*]] = call <vscale x 2 x i32> @llvm.vp.udiv.nxv2i32(<vscale x 2 x i32> [[LHS]], <vscale x 2 x i32> [[RHS]], [[TRUEMASK]], i32 [[VL]])
; CHECK-GE15: call void @llvm.vp.store.nxv2i32.p0(<vscale x 2 x i32> [[ADD]], ptr {{%.*}}, [[TRUEMASK]], i32 [[VL]])
; CHECK-LT15: call void @llvm.vp.store.nxv2i32.p0nxv2i32(<vscale x 2 x i32> [[ADD]], <vscale x 2 x i32>* {{%.*}}, [[TRUEMASK]], i32 [[VL]])
