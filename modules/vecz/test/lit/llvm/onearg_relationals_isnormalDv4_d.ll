; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -k test_isnormalDv4_d -vecz-simd-width=4 -vecz-choices=FullScalarization -S < %s | %filecheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)
declare spir_func <4 x i64> @_Z8isnormalDv4_d(<4 x double>)

define spir_kernel void @test_isnormalDv4_d(<4 x double> addrspace(1)* %in, <4 x i64> addrspace(1)* %out) {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds <4 x double>, <4 x double> addrspace(1)* %in, i64 %call
  %0 = load <4 x double>, <4 x double> addrspace(1)* %arrayidx, align 32
  %call1 = call spir_func <4 x i64> @_Z8isnormalDv4_d(<4 x double> %0)
  %arrayidx2 = getelementptr inbounds <4 x i64>, <4 x i64> addrspace(1)* %out, i64 %call
  store <4 x i64> %call1, <4 x i64> addrspace(1)* %arrayidx2, align 32
  ret void
}

; CHECK: define spir_kernel void @__vecz_v4_test_isnormalDv4_d
; CHECK: and <4 x i64>
; CHECK: and <4 x i64>
; CHECK: and <4 x i64>
; CHECK: and <4 x i64>
; CHECK: add nsw <4 x i64>
; CHECK: add nsw <4 x i64>
; CHECK: add nsw <4 x i64>
; CHECK: add nsw <4 x i64>
; CHECK: icmp ult <4 x i64>
; CHECK: icmp ult <4 x i64>
; CHECK: icmp ult <4 x i64>
; CHECK: icmp ult <4 x i64>
; CHECK: sext <4 x i1>
; CHECK: sext <4 x i1>
; CHECK: sext <4 x i1>
; CHECK: sext <4 x i1>
; CHECK: ret void
