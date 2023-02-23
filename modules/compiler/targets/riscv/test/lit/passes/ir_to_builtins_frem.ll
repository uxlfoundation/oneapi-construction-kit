; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --device "%riscv_device" %s --passes ir-to-builtins,verify -S | %filecheck %s

target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
target triple = "riscv64-unknown-unknown-elf"

; CHECK: call spir_func float @_Z4fmodff

define dso_local spir_kernel void @add(float addrspace(1)* readonly %in1, float addrspace(1)* readonly %in2, float addrspace(1)*  writeonly %out)  {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 noundef 0)
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %in1, i64 %call
  %0 = load float, float addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds float, float addrspace(1)* %in2, i64 %call
  %1 = load float, float addrspace(1)* %arrayidx1, align 4
  %call2 = frem float %0, %1
  %arrayidx3 = getelementptr inbounds float, float addrspace(1)* %out, i64 %call
  store float %call2, float addrspace(1)* %arrayidx3, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32 noundef)
