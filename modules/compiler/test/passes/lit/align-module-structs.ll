; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes align-module-structs,verify -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"

; Note: this custom(!) datalayout has preferred ABI alignments:
;   i16   - 64 bits
;   i32   - 64 bits
;   i64   - 128 bits
;   v4i32 - 64 bits
; We use these to trigger the struct alignment pass
target datalayout = "e-p:64:64:64-m:e-i16:64-i32:64-i64:128-v128:64"

; We don't need to update this struct with padding:
;   i32 is aligned to 4 bytes and is stored as 8 bytes as per our DL
;   i64 is aligned to 8 bytes (see above)
; CHECK-DAG: %structTyA = type { i32, i64, [4 x float] }
; CHECK-DAG: %structTyB = type { ptr, ptr }
; We need to update this struct with padding:
;   i16   is aligned to 2 bytes and is stored as 8 as per our DL
;   i8    is aligned to 8 bytes and is stored as 1 as per our DL
;   i16   is aligned to 8 bytes and is stored as 8 as per our DL
;   v4i32 is aligned to 8 bytes, and is stored as 16 as per its type size.
;           It must be aligned to 16 bytes as per SPIR DL -> insert 8 bytes.
;   v4i32 is aligned to 16 bytes given the previous elt
; CHECK-DAG: [[STyC:%structTyC.*]] = type { i16, i8, i16, [8 x i8], <4 x i32>, <4 x i32> }
; We don't need to update this struct with padding, despite what the preferred
; alignment says.
; CHECK-DAG: %structTyD = type { i16, %innerStructTyD }

%structTyA = type { i32, i64, [4 x float] }
%structTyB = type { i32*, i64* }
%structTyC = type { i16, i8, i16, <4 x i32>, <4 x i32> }
%innerStructTyD = type { i8 }
%structTyD = type { i16, %innerStructTyD }

; CHECK: @glob.1 = internal addrspace(3) global [[STyC]] undef
@glob = internal addrspace(3) global %structTyC undef

; CHECK-LABEL: define spir_kernel void @add() {
; CHECK: %a = alloca %structTyA{{(, align 16)?}}
; CHECK: %b = alloca %structTyB{{(, align 8)?}}
; CHECK: %c = alloca [[STyC]]{{(, align 8)?}}
; CHECK: %d = alloca %structTyD{{(, align 8)?}}
; CHECK: ret void
; CHECK: }
define spir_kernel void @add() {
  %a = alloca %structTyA
  %b = alloca %structTyB
  %c = alloca %structTyC
  %d = alloca %structTyD
  ret void
}
