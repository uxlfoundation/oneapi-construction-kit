; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --device "%default_device" --passes add-fp-control,verify -S %s  \
; RUN:   | %filecheck --check-prefix NOFTZ %s
; RUN: %muxc --device "%default_device" --passes "add-fp-control<ftz>,verify" -S %s  \
; RUN:   | %filecheck --check-prefix FTZ %s

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "armv7-linux-gnueabihf-elf"

; FTZ: define internal spir_kernel void @add(ptr addrspace(1) readonly %in1, ptr addrspace(1) readonly %in2, ptr addrspace(1) %out) [[ATTRS:#[0-9]+]] !test [[MD:\![0-9]+]] {

; Check that we've copied function and parameter attributes over, as well as
; parameter names for readability
; Check that we carry over all metadata, but steal the entry point metadata
; FTZ: define spir_kernel void @baz.host-fp-control(ptr addrspace(1) readonly %in1, ptr addrspace(1) readonly %in2, ptr addrspace(1) %out) [[WRAPPER_ATTRS:#[0-9]+]] !test [[MD]] {

; FTZ: llvm.arm.get.fpsc
; FTZ: llvm.arm.set.fpscr
; Check that we call the original function with the correct parameter attributes
; FTZ: call spir_kernel void @add(ptr addrspace(1) readonly %in1, ptr addrspace(1) readonly %in2, ptr addrspace(1) %out)
; FTZ: llvm.arm.set.fpscr

; On Arm32 we get the same result
; NOFTZ: llvm.arm.get.fpsc
; NOFTZ: llvm.arm.set.fpscr
; NOFTZ: llvm.arm.set.fpscr

define spir_kernel void @add(float addrspace(1)* readonly %in1, float addrspace(1)* readonly %in2, float addrspace(1)* %out) #0 !test !0 {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %in1, i64 %call
  %0 = load float, float addrspace(1)* %arrayidx, align 4
  %arrayidx2 = getelementptr inbounds float, float addrspace(1)* %in2, i64 %call
  %1 = load float, float addrspace(1)* %arrayidx2, align 4
  %add = fadd float %0, %1
  %arrayidx4 = getelementptr inbounds float, float addrspace(1)* %out, i64 %call
  store float %add, float addrspace(1)* %arrayidx4, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32 %x)

attributes #0 = { "foo"="bar" "mux-kernel"="entry-point" "mux-base-fn-name"="baz" }

!0 = !{!"test"}
