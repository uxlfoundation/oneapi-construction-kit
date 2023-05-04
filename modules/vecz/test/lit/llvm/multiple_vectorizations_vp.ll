; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; Check that veczc can vectorize a kernel multiple times in one go, with an
; equal width but with one enabling vector predication.
; RUN: %veczc -k add:1s,1sp -S < %s | %filecheck %s

declare spir_func i64 @_Z13get_global_idj(i32)

; CHECK: define spir_kernel void @add(
define spir_kernel void @add(ptr addrspace(1) %in1, ptr addrspace(1) %in2, ptr addrspace(1) %out) {
entry:
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx.in1 = getelementptr inbounds i32, ptr addrspace(1) %in1, i64 %idx
  %arrayidx.in2 = getelementptr inbounds i32, ptr addrspace(1) %in1, i64 %idx
  %in1.v = load i32, ptr addrspace(1) %arrayidx.in1, align 4
  %in2.v = load i32, ptr addrspace(1) %arrayidx.in2, align 4
  %add.v = add i32 %in1.v, %in2.v
  %arrayidx.out = getelementptr inbounds i32, ptr addrspace(1) %out, i64 %idx
  store i32 %add.v, ptr addrspace(1) %arrayidx.out
  ret void
}

; CHECK: define spir_kernel void @__vecz_nxv1_add

; CHECK: define spir_kernel void @__vecz_nxv1_vp_add
