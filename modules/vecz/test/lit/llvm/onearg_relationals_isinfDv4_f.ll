; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -k test_isinfDv4_f -vecz-simd-width=4 -vecz-choices=FullScalarization -S < %s | %filecheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)
declare spir_func <4 x i32> @_Z5isinfDv4_f(<4 x float>)

define spir_kernel void @test_isinfDv4_f(<4 x float> addrspace(1)* %in, <4 x i32> addrspace(1)* %out) {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds <4 x float>, <4 x float> addrspace(1)* %in, i64 %call
  %0 = load <4 x float>, <4 x float> addrspace(1)* %arrayidx, align 16
  %call1 = call spir_func <4 x i32> @_Z5isinfDv4_f(<4 x float> %0)
  %arrayidx2 = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %out, i64 %call
  store <4 x i32> %call1, <4 x i32> addrspace(1)* %arrayidx2, align 16
  ret void
}

; CHECK: define spir_kernel void @__vecz_v4_test_isinfDv4_f
; CHECK: and <4 x i32>
; CHECK: and <4 x i32>
; CHECK: and <4 x i32>
; CHECK: and <4 x i32>
; CHECK: icmp eq <4 x i32>
; CHECK: icmp eq <4 x i32>
; CHECK: icmp eq <4 x i32>
; CHECK: icmp eq <4 x i32>
; CHECK: sext <4 x i1>
; CHECK: sext <4 x i1>
; CHECK: sext <4 x i1>
; CHECK: sext <4 x i1>
; CHECK: ret void
