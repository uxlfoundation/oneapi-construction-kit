; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; REQUIRES: llvm-13+
; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k mask_varying -vecz-scalable -vecz-simd-width=4 -S < %s | %filecheck %t

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
; CHECK: define spir_kernel void @__vecz_nxv4_mask_varying
; CHECK: [[idx0:%.*]] = call <vscale x 16 x i32> @llvm.experimental.stepvector.nxv16i32()
; CHECK: [[idx1:%.*]] = lshr <vscale x 16 x i32> [[idx0]], shufflevector (<vscale x 16 x i32> insertelement (<vscale x 16 x i32> {{(undef|poison)}}, i32 2, {{(i32|i64)}} 0), <vscale x 16 x i32> {{(undef|poison)}}, <vscale x 16 x i32> zeroinitializer)

; Note that since we just did a lshr 2 on the input of the extend, it doesn't
; make any difference whether it's a zext or sext, but LLVM 16 prefers zext.
; CHECK: [[idx2:%.*]] = {{s|z}}ext <vscale x 16 x i32> [[idx1]] to <vscale x 16 x i64>

; CHECK-GE15: [[t1:%.*]] = getelementptr inbounds i8, ptr {{.*}}, <vscale x 16 x i64> [[idx2]]
; CHECK-LT15: [[t1:%.*]] = getelementptr inbounds i8, i8* {{.*}}, <vscale x 16 x i64> [[idx2]]
; CHECK-GE15: [[t2:%.*]] = call <vscale x 16 x i8> @llvm.masked.gather.nxv16i8.nxv16p0(<vscale x 16 x ptr> [[t1]],
; CHECK-LT15: [[t2:%.*]] = call <vscale x 16 x i8> @llvm.masked.gather.nxv16i8.nxv16p0i8(<vscale x 16 x i8*> [[t1]],
; CHECK: [[splat:%.*]] = trunc <vscale x 16 x i8> [[t2]] to <vscale x 16 x i1>
; CHECK-GE15: call void @__vecz_b_masked_store16_u6nxv16ju3ptru6nxv16b(<vscale x 16 x i32> {{.*}}, ptr %arrayidxz, <vscale x 16 x i1> [[splat]])
; CHECK-LT15: [[t3:%.*]] = bitcast <4 x i32>* %arrayidxz to <vscale x 16 x i32>*
; CHECK-LT15: call void @__vecz_b_masked_store16_u6nxv16jPu6nxv16ju6nxv16b(<vscale x 16 x i32> {{.*}}, <vscale x 16 x i32>* [[t3]], <vscale x 16 x i1> [[splat]])

}

declare spir_func i64 @_Z13get_global_idj(i32)
declare <4 x i32> @__vecz_b_masked_load4_Dv4_jPDv4_jDv4_b(<4 x i32>*, <4 x i1>)
