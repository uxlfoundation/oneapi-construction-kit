; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %muxc --passes align-module-structs,verify -S %s | %filecheck %t

target triple = "spir64-unknown-unknown"

; Note: this custom(!) datalayout has preferred ABI alignments:
;   i32 - 64 bits
;   i64 - 128 bits
; We use these to trigger the struct alignment pass
target datalayout = "e-p:64:64:64-m:e-i32:64-i64:128"

; CHECK: %structTyA.0 = type { i32, [8 x i8], i64, [4 x float] }
; CHECK-GE15: %structTyB.1 = type { ptr, ptr }
; CHECK-LT15: %structTyB.1 = type { i32*, i64* }

%structTyA = type { i32, i64, [4 x float] }
%structTyB = type { i32*, i64* }

; CHECK: @glob.1 = internal addrspace(3) global %structTyA.0 undef
@glob = internal addrspace(3) global %structTyA undef

; CHECK-LABEL: define spir_kernel void @add() {
; CHECK: %a = alloca %structTyA.0{{(, align 16)?}}
; CHECK: %b = alloca %structTyB.1{{(, align 8)?}}
; CHECK: ret void
; CHECK: }
define spir_kernel void @add() {
  %a = alloca %structTyA
  %b = alloca %structTyB
  ret void
}
