; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; REQUIRES: llvm-13+
; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -vecz-target-triple="riscv64-unknown-unknown" -vecz-scalable -vecz-simd-width=4 -vecz-passes=packetizer -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @f(<4 x i32> addrspace(1)* %in, <4 x i32> addrspace(1)* %out) {
entry:
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  %in.ptr = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %in, i64 %gid
  %in.data = load <4 x i32>, <4 x i32> addrspace(1)* %in.ptr
  %out.data = shufflevector <4 x i32> %in.data, <4 x i32> undef, <4 x i32> <i32 3, i32 2, i32 1, i32 0>
  %out.ptr = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %out, i64 %gid
  store <4 x i32> %out.data, <4 x i32> addrspace(1)* %out.ptr, align 32
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32) #1

; It checks that a single-operand shuffle that doesn't change the length is packetized to a gather intrinsic.
; CHECK: define spir_kernel void @__vecz_nxv4_f({{.*}}) {{.*}} {
; CHECK: entry:
; CHECK:  %[[DATA:.+]] = load <vscale x 16 x i32>, {{(<vscale x 16 x i32> addrspace\(1\)\*)|(ptr addrspace\(1\))}} %{{.*}}
; CHECK-LT15:  %[[GATHER:.+]] = call <vscale x 16 x i32> @llvm.riscv.vrgather.vv.nxv16i32.i64(<vscale x 16 x i32> %[[DATA]], <vscale x 16 x i32> %{{.+}}, i64 %{{.+}})
; CHECK-GE15:  %[[GATHER:.+]] = call <vscale x 16 x i32> @llvm.riscv.vrgather.vv.nxv16i32.i64(<vscale x 16 x i32> undef, <vscale x 16 x i32> %[[DATA]], <vscale x 16 x i32> %{{.+}}, i64 %{{.+}})
; CHECK:  store <vscale x 16 x i32> %[[GATHER]]
; CHECK:  ret void
; CHECK: }
