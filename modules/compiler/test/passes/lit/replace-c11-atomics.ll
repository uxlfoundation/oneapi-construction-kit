; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes replace-c11-atomic-funcs,verify -S %s -opaque-pointers | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

declare spir_func float @_Z25atomic_fetch_add_explicitPU3AS1Vff(ptr addrspace(1), float)

define spir_kernel void @foo(ptr addrspace(1) %in) {
; CHECK: = atomicrmw fadd ptr addrspace(1) %in, float 1.000000e+00 monotonic, align 4
  %a = call spir_func float @_Z25atomic_fetch_add_explicitPU3AS1Vff(ptr addrspace(1) %in, float 1.0)
  ret void
}

!opencl.ocl.version = !{!0}

!0 = !{i32 3, i32 0}
