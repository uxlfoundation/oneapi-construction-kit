; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: env CA_RISCV_VF=8 %muxc --device "%riscv_device" --passes "run-vecz" -S %s \
; RUN:   | %filecheck %s --check-prefixes CHECK,CHECK-8
; RUN: env CA_RISCV_VF=1,S %muxc --device "%riscv_device" --passes "run-vecz" -S %s \
; RUN:   | %filecheck %s --check-prefixes CHECK,CHECK-1S
; RUN: env CA_RISCV_VF=1,S,VP %muxc --device "%riscv_device" --passes "run-vecz" -S %s \
; RUN:   | %filecheck %s --check-prefixes CHECK,CHECK-1S-VP
; RUN: env CA_RISCV_VF=1,S,VVP %muxc --device "%riscv_device" --passes "run-vecz" -S %s \
; RUN:   | %filecheck %s --check-prefixes CHECK,CHECK-1S,CHECK-1S-VP

; An easy shared positive check as a test with only one CHECK-NOT is invalid
; CHECK: target triple
target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
target triple = "riscv64-unknown-unknown-elf"

; CHECK-8: define spir_kernel void @__vecz_v8_foo(
; CHECK-8: = load <8 x i32>
; CHECK-8: = add nsw <8 x i32>
; CHECK-8: store <8 x i32>

; CHECK-1S: define spir_kernel void @__vecz_nxv1_foo(
; CHECK-1S: = load <vscale x 1 x i32>
; CHECK-1S: = add nsw <vscale x 1 x i32>
; CHECK-1S: store <vscale x 1 x i32>

; CHECK-1S-VP: define spir_kernel void @__vecz_nxv1_vp_foo(
; CHECK-1S-VP: = call <vscale x 1 x i32> @llvm.vp.load.nxv1i32.p1(
; CHECK-1S-VP: = call <vscale x 1 x i32> @llvm.vp.add.nxv1i32(<vscale x 1 x i32>
; CHECK-1S-VP: call void @llvm.vp.store.nxv1i32.p1(<vscale x 1 x i32>
define spir_kernel void @foo(i32 addrspace(1)* %a, i32 addrspace(1)* %z) #0 {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %a, i64 %call
  %x = load i32, i32 addrspace(1)* %arrayidx, align 4
  %add = add nsw i32 %x, 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %z, i64 %call
  store i32 %add, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}

; optnone means don't optimize!
; CHECK-NOT __vecz_{{.*}}_bar
define spir_kernel void @bar(i32 addrspace(1)* %a, i32 addrspace(1)* %z) #1 {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %a, i64 %call
  %x = load i32, i32 addrspace(1)* %arrayidx, align 4
  %add = add nsw i32 %x, 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %z, i64 %call
  store i32 %add, i32 addrspace(1)* %arrayidx1, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

attributes #0 = { "mux-kernel"="entry-point" }
attributes #1 = { optnone noinline "mux-kernel"="entry-point" }
