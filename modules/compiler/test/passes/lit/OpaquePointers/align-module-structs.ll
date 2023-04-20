; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes align-module-structs,verify -S %s %flag | %filecheck %s

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
;   i16   is aligned to 8 bytes and is stored as 8 as per our DL
;   i8    is aligned to 8 bytes and is stored as 1 as per our DL
;   i16   is aligned to 8 bytes and is stored as 8 as per our DL
;   v4i32 is aligned to 8 bytes, and is stored as 16 as per its type size.
;           It must be aligned to 16 bytes as per SPIR DL -> insert 8 bytes.
;   v4i32 is aligned to 16 bytes given the previous elt
; CHECK-DAG: [[STyC:%structTyC.*]] = type { i16, i8, i16, [8 x i8], <4 x i32>, <4 x i32> }
; We need to update this struct with padding:
;   i16   is aligned to 8 bytes and is stored as 8 as per our DL
;   i8    is aligned to 8 bytes and is stored as 1 as per our DL
;   i16   is aligned to 8 bytes and is stored as 8 as per our DL
;   v4i32 is aligned to 8 bytes, and is stored as 16 as per its type size.
;           It must be aligned to 16 bytes as per SPIR DL -> insert 8 bytes.
;   i16   is aligned to 8 bytes and is stored as 8 as per our DL
;   v4i32 is aligned to 8 bytes, and is stored as 16 as per its type size.
;           It must be aligned to 16 bytes as per SPIR DL -> insert 8 bytes.
; CHECK-DAG: [[STyD:%structTyD.*]] = type { i16, i8, i16, [8 x i8], <4 x i32>, i16, [8 x i8], <4 x i32> }

%structTyA = type { i32, i64, [4 x float] }
%structTyB = type { i32*, i64* }
%structTyC = type { i16, i8, i16, <4 x i32>, <4 x i32> }
%structTyD = type { i16, i8, i16, <4 x i32>, i16, <4 x i32> }

; CHECK: @glob.1 = internal addrspace(3) global [[STyC]] undef
@glob = internal addrspace(3) global %structTyC undef

; CHECK-LABEL: define spir_kernel void @allocas() {
; CHECK: %a = alloca %structTyA, align 16
; CHECK: %b = alloca %structTyB, align 8
; CHECK: %c = alloca [[STyC]], align 8
; CHECK: %d = alloca [[STyD]], align 8
; CHECK: ret void
; CHECK: }
define spir_kernel void @allocas() {
  %a = alloca %structTyA
  %b = alloca %structTyB
  %c = alloca %structTyC
  %d = alloca %structTyD
  ret void
}

; CHECK-LABEL: define spir_kernel void @load_val(ptr %p) {
; CHECK: load [[STyC]], ptr %p, align 8
define spir_kernel void @load_val(i8* %p) {
  %a = load %structTyC, i8* %p
  ret void
}

; CHECK-LABEL: define spir_kernel void @load_ptr() {
; CHECK: load i8, ptr addrspace(3) @glob.1
define spir_kernel void @load_ptr() {
  %a = load i8, %structTyC addrspace(3)* @glob
  ret void
}

; CHECK-LABEL: define spir_kernel void @store_val(ptr %p) {
; CHECK: store [[STyC]] zeroinitializer, ptr %p, align 8
define spir_kernel void @store_val(i8* %p) {
  store %structTyC zeroinitializer, i8* %p
  ret void
}

; CHECK-LABEL: define spir_kernel void @store_ptr() {
; CHECK: store i8 0, ptr addrspace(3) @glob.1
define spir_kernel void @store_ptr() {
  store i8 0, %structTyC addrspace(3)* @glob
  ret void
}

; CHECK-LABEL: define spir_kernel void @gep_src_elt_ty() {
; CHECK: = getelementptr [[STyC]], ptr addrspace(3) @glob.1, i64 0
define spir_kernel void @gep_src_elt_ty() {
  %v = getelementptr %structTyC, %structTyC addrspace(3)* @glob, i64 0
  ret void
}

; CHECK-LABEL: define spir_kernel void @gep_ptr_ty() {
; CHECK: = getelementptr i8, ptr addrspace(3) @glob.1, i64 0
define spir_kernel void @gep_ptr_ty() {
  %v = getelementptr i8, %structTyC addrspace(3)* @glob, i64 0
  ret void
}

; CHECK-LABEL: define spir_kernel void @cmpxchg() {
; CHECK: = cmpxchg ptr addrspace(3) @glob.1, i32 0, i32 1 acq_rel monotonic
define spir_kernel void @cmpxchg() {
  %v = cmpxchg %structTyC addrspace(3)* @glob, i32 0, i32 1 acq_rel monotonic
  ret void
}

; CHECK-LABEL: define spir_kernel void @atomicrmw() {
; CHECK: = atomicrmw add ptr addrspace(3) @glob.1, i32 0 acquire
define spir_kernel void @atomicrmw() {
  %v = atomicrmw add %structTyC addrspace(3)* @glob, i32 0 acquire
  ret void
}

; CHECK-LABEL: define spir_kernel void @addrspacecast() {
; CHECK: = addrspacecast ptr addrspace(3) @glob.1 to ptr addrspace(2)
define spir_kernel void @addrspacecast() {
  %v = addrspacecast %structTyC addrspace(3)* @glob to %structTyC addrspace(2)*
  ret void
}

; CHECK-LABEL: define spir_kernel void @ptrtoint() {
; CHECK: = ptrtoint ptr addrspace(3) @glob.1 to i32
define spir_kernel void @ptrtoint() {
  %v = ptrtoint %structTyC addrspace(3)* @glob to i32
  ret void
}

; CHECK-LABEL: define spir_kernel void @select(i1 %cmp) {
; CHECK: = select i1 %cmp, ptr addrspace(3) @glob.1, ptr addrspace(3) null
define spir_kernel void @select(i1 %cmp) {
  %v = select i1 %cmp, %structTyC addrspace(3)* @glob, %structTyC addrspace(3)* null
  ret void
}

; CHECK-LABEL: define spir_kernel void @phi() {
; CHECK: = phi ptr addrspace(3) [ @glob.1, %entry ]
define spir_kernel void @phi() {
entry:
  br label %if
if:
  %v = phi %structTyC addrspace(3)* [ @glob, %entry ]
  ret void
}

declare i32 @ext(%structTyC addrspace(3)*)

; CHECK-LABEL: define spir_kernel void @call() {
; CHECK: = call i32 @ext(ptr addrspace(3) @glob.1)
define spir_kernel void @call() {
  %v = call i32 @ext(%structTyC addrspace(3)* @glob)
  ret void
}
