; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes align-module-structs,verify -S %s %flag | %filecheck %s

target triple = "spir64-unknown-unknown"

; Note: this custom(!) datalayout has preferred ABI alignments:
;   i32 - 64 bits
;   i64 - 128 bits
; We use these to trigger the struct alignment pass
target datalayout = "e-p:64:64:64-m:e-i32:64-i64:128"

; CHECK: %structTyA.0 = type { i32, [8 x i8], i64, [4 x float] }
; FIXME: We shouldn't need to remap this type as it's equivalent to the
; original. See CA-4348.
; CHECK: %structTyB.1 = type { ptr, ptr }

%structTyA = type { i32, i64, [4 x float] }
%structTyB = type { i32*, i64* }

; CHECK: @glob.1 = internal addrspace(3) global %structTyA.0 undef
@glob = internal addrspace(3) global %structTyA undef

; CHECK-LABEL: define spir_kernel void @allocas() {
; CHECK: %a = alloca %structTyA.0, align 16
; CHECK: %b = alloca %structTyB.1, align 8
; CHECK: ret void
; CHECK: }
define spir_kernel void @allocas() {
  %a = alloca %structTyA
  %b = alloca %structTyB
  ret void
}

; CHECK-LABEL: define spir_kernel void @load_val(ptr %p) {
; CHECK: load %structTyA.0, ptr %p, align 16
define spir_kernel void @load_val(i8* %p) {
  %a = load %structTyA, i8* %p
  ret void
}

; CHECK-LABEL: define spir_kernel void @load_ptr() {
; CHECK: load i8, ptr addrspace(3) @glob.1
define spir_kernel void @load_ptr() {
  %a = load i8, %structTyA addrspace(3)* @glob
  ret void
}

; CHECK-LABEL: define spir_kernel void @store_val(ptr %p) {
; CHECK: store %structTyA.0 zeroinitializer, ptr %p, align 16
define spir_kernel void @store_val(i8* %p) {
  store %structTyA zeroinitializer, i8* %p
  ret void
}

; CHECK-LABEL: define spir_kernel void @store_ptr() {
; CHECK: store i8 0, ptr addrspace(3) @glob.1
define spir_kernel void @store_ptr() {
  store i8 0, %structTyA addrspace(3)* @glob
  ret void
}

; CHECK-LABEL: define spir_kernel void @gep_src_elt_ty() {
; CHECK: = getelementptr %structTyA.0, ptr addrspace(3) @glob.1, i64 0
define spir_kernel void @gep_src_elt_ty() {
  %v = getelementptr %structTyA, %structTyA addrspace(3)* @glob, i64 0
  ret void
}

; CHECK-LABEL: define spir_kernel void @gep_ptr_ty() {
; CHECK: = getelementptr i8, ptr addrspace(3) @glob.1, i64 0
define spir_kernel void @gep_ptr_ty() {
  %v = getelementptr i8, %structTyA addrspace(3)* @glob, i64 0
  ret void
}

; CHECK-LABEL: define spir_kernel void @cmpxchg() {
; CHECK: = cmpxchg ptr addrspace(3) @glob.1, i32 0, i32 1 acq_rel monotonic
define spir_kernel void @cmpxchg() {
  %v = cmpxchg %structTyA addrspace(3)* @glob, i32 0, i32 1 acq_rel monotonic
  ret void
}

; CHECK-LABEL: define spir_kernel void @atomicrmw() {
; CHECK: = atomicrmw add ptr addrspace(3) @glob.1, i32 0 acquire
define spir_kernel void @atomicrmw() {
  %v = atomicrmw add %structTyA addrspace(3)* @glob, i32 0 acquire
  ret void
}

; CHECK-LABEL: define spir_kernel void @addrspacecast() {
; CHECK: = addrspacecast ptr addrspace(3) @glob.1 to ptr addrspace(2)
define spir_kernel void @addrspacecast() {
  %v = addrspacecast %structTyA addrspace(3)* @glob to %structTyA addrspace(2)*
  ret void
}

; CHECK-LABEL: define spir_kernel void @ptrtoint() {
; CHECK: = ptrtoint ptr addrspace(3) @glob.1 to i32
define spir_kernel void @ptrtoint() {
  %v = ptrtoint %structTyA addrspace(3)* @glob to i32
  ret void
}

; CHECK-LABEL: define spir_kernel void @select(i1 %cmp) {
; CHECK: = select i1 %cmp, ptr addrspace(3) @glob.1, ptr addrspace(3) null
define spir_kernel void @select(i1 %cmp) {
  %v = select i1 %cmp, %structTyA addrspace(3)* @glob, %structTyA addrspace(3)* null
  ret void
}

; CHECK-LABEL: define spir_kernel void @phi() {
; CHECK: = phi ptr addrspace(3) [ @glob.1, %entry ]
define spir_kernel void @phi() {
entry:
  br label %if
if:
  %v = phi %structTyA addrspace(3)* [ @glob, %entry ]
  ret void
}

declare i32 @ext(%structTyA addrspace(3)*)

; CHECK-LABEL: define spir_kernel void @call() {
; CHECK: = call i32 @ext(ptr addrspace(3) @glob.1)
define spir_kernel void @call() {
  %v = call i32 @ext(%structTyA addrspace(3)* @glob)
  ret void
}
