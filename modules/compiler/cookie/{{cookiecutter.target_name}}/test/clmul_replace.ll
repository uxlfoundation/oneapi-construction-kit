; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --device "RefSi M1 Tutorial" %s --passes "require<builtin-info>,optimal-builtin-replace,verify" -S | %filecheck %s

target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
target triple = "riscv64-unknown-unknown-elf"

; CHECK:   call i64 @llvm.riscv.clmul.i64
; CHECK:   call i64 @llvm.riscv.clmulh.i64
; CHECK:   call i64 @llvm.riscv.clmulr.i64

; Function Attrs: norecurse nounwind
define spir_kernel void @do_clmul(ptr addrspace(1) %a, ptr addrspace(1) %b, ptr addrspace(1) %z) {
entry:
  %0 = load i64, ptr addrspace(1) %a
  %1 = load i64, ptr addrspace(1) %b
  %call = tail call spir_func i64 @_Z5clmulll(i64 %0, i64 %1)
  %call4 = tail call spir_func i64 @_Z6clmulhll(i64 %0, i64 %1)
  %add = add nsw i64 %call4, %call
  %call7 = tail call spir_func i64 @_Z6clmulrll(i64 %0, i64 %1)
  %add8 = add nsw i64 %add, %call7
  store i64 %add8, ptr addrspace(1) %z
  ret void
}

declare spir_func i64 @_Z5clmulll(i64, i64)

declare spir_func i64 @_Z6clmulhll(i64, i64)

declare spir_func i64 @_Z6clmulrll(i64, i64)

