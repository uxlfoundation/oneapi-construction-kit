; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -k splat -vecz-simd-width=4 -vecz-passes=scalarize -vecz-choices=FullScalarization -S < %s | %filecheck %s

; ModuleID = 'kernel.opencl'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define dso_local spir_kernel void @splat(float addrspace(1)* %data, float addrspace(1)* %out) {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 noundef 0)
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %data, i64 %call
  %0 = load float, float addrspace(1)* %arrayidx, align 4
  %splat.splatinsert = insertelement <4 x float> poison, float %0, i64 0
  %splat.splat = shufflevector <4 x float> %splat.splatinsert, <4 x float> poison, <4 x i32> zeroinitializer
  %call1 = tail call spir_func float @not_scalarizable(<4 x float> noundef %splat.splat)
  %arrayidx2 = getelementptr inbounds float, float addrspace(1)* %out, i64 %call
  store float %call1, float addrspace(1)* %arrayidx2, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32 noundef)
declare spir_func float @not_scalarizable(<4 x float> noundef)

; It checks that the scalarizer turns the original vector splat back into a vector splat,
; instead of a series of insertelement instructions.
; CHECK: void @__vecz_v4_splat({{.*}})
; CHECK: entry:
; CHECK:   %[[LD:.*]] = load float
; CHECK:   %[[INS0:.*]] = insertelement <4 x float> {{undef|poison}}, float %[[LD]], {{i32|i64}} 0
; CHECK-NOT: %{{.*}} = insertelement <4 x float> %{{.*}}, float %[[LD]], {{i32|i64}} 1
; CHECK-NOT: %{{.*}} = insertelement <4 x float> %{{.*}}, float %[[LD]], {{i32|i64}} 2
; CHECK-NOT: %{{.*}} = insertelement <4 x float> %{{.*}}, float %[[LD]], {{i32|i64}} 3
; CHECK:   %[[SPLAT:.*]] = shufflevector <4 x float> %[[INS0]], <4 x float> {{undef|poison}}, <4 x i32> zeroinitializer
; CHECK:   %{{.*}} = tail call spir_func float @not_scalarizable(<4 x float> noundef %[[SPLAT]])
