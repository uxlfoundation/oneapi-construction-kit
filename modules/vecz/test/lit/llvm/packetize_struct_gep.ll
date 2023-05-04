; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -k test -vecz-simd-width=4 -S < %s | %filecheck %s

; ModuleID = 'kernel.opencl'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

%struct.T = type { i32, i8, float, i64 }

; Function Attrs: nounwind
define spir_kernel void @test(%struct.T addrspace(1)* %in, %struct.T addrspace(1)* %out, i32 addrspace(1)* %offsets) {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %offsets, i64 %call
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %conv = sext i32 %0 to i64
  %add = add i64 %conv, %call
  %c = getelementptr inbounds %struct.T, %struct.T addrspace(1)* %in, i64 %add, i32 2
  %1 = load float, float addrspace(1)* %c, align 8
  %c3 = getelementptr inbounds %struct.T, %struct.T addrspace(1)* %out, i64 %add, i32 2
  store float %1, float addrspace(1)* %c3, align 8
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; Check if we can packetize GEPs on structs
; Note that we only need to packetize the non-uniform operands..
; CHECK: define spir_kernel void @__vecz_v4_test
; CHECK: getelementptr inbounds %struct.T, ptr addrspace(1) %{{.+}}, <4 x i64> %{{.+}}, i32 2
; CHECK: getelementptr inbounds %struct.T, ptr addrspace(1) %{{.+}}, <4 x i64> %{{.+}}, i32 2
