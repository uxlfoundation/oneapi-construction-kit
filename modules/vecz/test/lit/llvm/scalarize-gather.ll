; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -k splat -vecz-simd-width=4 -vecz-passes=scalarize -vecz-choices=FullScalarization -S < %s | %filecheck %s

; ModuleID = 'kernel.opencl'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define dso_local spir_kernel void @splat(i32 addrspace(1)* %data, i32 addrspace(1)* %out) {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 noundef 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %data, i64 %call
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %splat.splatinsert = insertelement <4 x i32> poison, i32 %0, i64 0
  %splat.splat = shufflevector <4 x i32> %splat.splatinsert, <4 x i32> poison, <4 x i32> zeroinitializer
  %add = add <4 x i32> %splat.splat, <i32 2, i32 3, i32 5, i32 7>
  %call1 = tail call spir_func i32 @not_scalarizable(<4 x i32> noundef %add)
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %call
  store i32 %call1, i32 addrspace(1)* %arrayidx2, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32 noundef)
declare spir_func i32 @not_scalarizable(<4 x i32> noundef)

; It checks that the scalarizer scalarizes the add and reconstructs the vector
; using insert element instructions to be consumed by the unscalarizable
; function.
; CHECK: void @__vecz_v4_splat({{.*}})
; CHECK: entry:
; CHECK:   %[[LD:.*]] = load i32
; CHECK:   %[[ADD0:.*]] = add i32 %[[LD]]
; CHECK:   %[[ADD1:.*]] = add i32 %[[LD]]
; CHECK:   %[[ADD2:.*]] = add i32 %[[LD]]
; CHECK:   %[[ADD3:.*]] = add i32 %[[LD]]
; CHECK:   %[[INS0:.*]] = insertelement <4 x i32> {{undef|poison}}, i32 %[[ADD0]], i32 0
; CHECK:   %[[INS1:.+]] = insertelement <4 x i32> %[[INS0]], i32 %[[ADD1]], i32 1
; CHECK:   %[[INS2:.+]] = insertelement <4 x i32> %[[INS1]], i32 %[[ADD2]], i32 2
; CHECK:   %[[INS3:.+]] = insertelement <4 x i32> %[[INS2]], i32 %[[ADD3]], i32 3
; CHECK-NOT: shufflevector <4 x i32>
; CHECK:   %{{.*}} = tail call spir_func i32 @not_scalarizable(<4 x i32> noundef %[[INS3]])
