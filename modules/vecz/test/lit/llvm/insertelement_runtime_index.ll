; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -k runtime_index -vecz-simd-width=4 -vecz-choices=FullScalarization -S < %s | %filecheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @runtime_index(<4 x i32>* %in, <4 x i32>* %out, i32* %index) {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32>* %in, i64 %call
  %0 = load <4 x i32>, <4 x i32>* %arrayidx
  %arrayidx1 = getelementptr inbounds <4 x i32>, <4 x i32>* %out, i64 %call
  store <4 x i32> %0, <4 x i32>* %arrayidx1
  %arrayidx2 = getelementptr inbounds i32, i32* %index, i64 %call
  %1 = load i32, i32* %arrayidx2
  %arrayidx3 = getelementptr inbounds <4 x i32>, <4 x i32>* %out, i64 %call
  %vecins = insertelement <4 x i32> %0, i32 42, i32 %1
  store <4 x i32> %vecins, <4 x i32>* %arrayidx3
  ret void
}

; CHECK: define spir_kernel void @__vecz_v4_runtime_index

; Four icmps and selects
; CHECK: icmp eq <4 x i32> %{{.+}}, zeroinitializer
; CHECK: select <4 x i1> %{{.+}}, <4 x i32> <i32 42, i32 42, i32 42, i32 42>
; CHECK: icmp eq <4 x i32> %{{.+}}, <i32 1, i32 1, i32 1, i32 1>
; CHECK: select <4 x i1> %{{.+}}, <4 x i32> <i32 42, i32 42, i32 42, i32 42>
; CHECK: icmp eq <4 x i32> %{{.+}}, <i32 2, i32 2, i32 2, i32 2>
; CHECK: select <4 x i1> %{{.+}}, <4 x i32> <i32 42, i32 42, i32 42, i32 42>
; CHECK: icmp eq <4 x i32> %{{.+}}, <i32 3, i32 3, i32 3, i32 3>
; CHECK: select <4 x i1> %{{.+}}, <4 x i32> <i32 42, i32 42, i32 42, i32 42>

; Four stores
; CHECK: store <4 x i32>
; CHECK: store <4 x i32>
; CHECK: store <4 x i32>
; CHECK: store <4 x i32>
; CHECK: ret void
