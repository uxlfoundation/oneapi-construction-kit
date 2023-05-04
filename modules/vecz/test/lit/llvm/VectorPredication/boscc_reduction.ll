; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; REQUIRES: llvm-13+
; RUN: %veczc -k foo -vecz-scalable -vecz-simd-width=2 -vecz-choices=VectorPredication -S < %s | %filecheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @foo(float addrspace(1)* nocapture readonly %a, i32 addrspace(1)* nocapture %out) {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0) #2
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %a, i64 %call
  %0 = load float, float addrspace(1)* %arrayidx, align 4
  %cmp = fcmp oeq float %0, 0.000000e+00
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %call
  %1 = load i32, i32 addrspace(1)* %arrayidx1, align 4
  %add = add nsw i32 %1, 42
  store i32 %add, i32 addrspace(1)* %arrayidx1, align 4
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  ret void
}

; CHECK: define spir_kernel void @__vecz_nxv2_vp_foo(ptr addrspace(1) nocapture readonly %a, ptr addrspace(1) nocapture %out)
; CHECK:  [[CMP:%.*]] = fcmp oeq <vscale x 2 x float> %{{.*}}, zeroinitializer
; CHECK:  [[INS:%.*]] = insertelement <vscale x 2 x i32> poison, i32 [[VL:%.*]], {{(i32|i64)}} 0
; CHECK:  [[SPLAT:%.*]] = shufflevector <vscale x 2 x i32> [[INS]], <vscale x 2 x i32> poison, <vscale x 2 x i32> zeroinitializer
; CHECK:  [[IDX:%.*]] = call <vscale x 2 x i32> @llvm.experimental.stepvector.nxv2i32()
; CHECK:  [[MASK:%.*]] = icmp ult <vscale x 2 x i32> [[IDX]], [[SPLAT]]
; CHECK:  [[INP:%.*]] = select <vscale x 2 x i1> [[MASK]], <vscale x 2 x i1> [[CMP]], <vscale x 2 x i1> zeroinitializer
; CHECK:  %{{.*}} = call i1 @llvm.vector.reduce.or.nxv2i1(<vscale x 2 x i1> [[INP]])
