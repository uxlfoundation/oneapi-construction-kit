; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k foo3 -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @foo1(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 %call
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %call
  store i32 %0, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @foo2(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  call spir_kernel void @foo1(i32 addrspace(1)* %in, i32 addrspace(1)* %out)
  ret void
}

define spir_kernel void @foo3(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  call spir_kernel void @foo2(i32 addrspace(1)* %in, i32 addrspace(1)* %out)
  ret void
}

; CHECK-GE15: define spir_kernel void @__vecz_v4_foo3(ptr addrspace(1) %in, ptr addrspace(1) %out)
; CHECK-LT15: define spir_kernel void @__vecz_v4_foo3(i32 addrspace(1)* %in, i32 addrspace(1)* %out)
; CHECK-NOT: call spir_kernel
; CHECK: call spir_func i64 @_Z13get_global_idj(i32 0)
; CHECK-GE15: load <4 x i32>, ptr addrspace(1) %{{.+}}, align 4
; CHECK-LT15: load <4 x i32>, <4 x i32> addrspace(1)* %{{.+}}, align 4
; CHECK-GE15: store <4 x i32> %{{.+}}, ptr addrspace(1) %{{.+}}, align 4
; CHECK-LT15: store <4 x i32> %{{.+}}, <4 x i32> addrspace(1)* %{{.+}}, align 4
; CHECK: ret void
