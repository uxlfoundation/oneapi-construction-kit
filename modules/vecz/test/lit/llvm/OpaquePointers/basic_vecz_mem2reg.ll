; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; Note: *not* running LLVM's mem2reg pass as before LLVM 15 it crashes for the
; same reason we used to!
; RUN: %veczc -vecz-passes=vecz-mem2reg -vecz-simd-width=4 %flag -S < %s | %filecheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @load_store_type_mismatch_no_bitcast(ptr addrspace(1) %p) {
  %data = alloca i32, align 4
  %1 = tail call spir_func i64 @_Z13get_global_idj(i32 0) #4
  %2 = getelementptr inbounds i32, ptr addrspace(1) %p, i64 %1
  %3 = load i32, ptr addrspace(1) %2, align 4
  store i32 %3, ptr %data, align 4
  %4 = load <2 x i16>, ptr %data, align 2
  ret void
}

define spir_kernel void @load_type_size_mismatch_no_bitcast(ptr addrspace(1) %p) {
  %data = alloca i32, align 4
  %1 = tail call spir_func i64 @_Z13get_global_idj(i32 0) #4
  %2 = getelementptr inbounds i32, ptr addrspace(1) %p, i64 %1
  %3 = load i32, ptr addrspace(1) %2, align 4
  store i32 %3, ptr %data, align 4
  %4 = load i16, ptr %data, align 2
  ret void
}

define spir_kernel void @store_type_size_mismatch_no_bitcast(ptr addrspace(1) %p) {
  %data = alloca i32, align 4
  %1 = tail call spir_func i64 @_Z13get_global_idj(i32 0) #4
  %2 = getelementptr inbounds i16, ptr addrspace(1) %p, i64 %1
  %3 = load i16, ptr addrspace(1) %2, align 4
  store i16 %3, ptr %data, align 2
  %4 = load i32, ptr %data, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; CHECK: define spir_kernel void @__vecz_v4_load_store_type_mismatch_no_bitcast(ptr addrspace(1) %p)
; CHECK-NOT: alloca i32
; CHECK:  %3 = load i32, ptr addrspace(1) %2, align 4
; CHECK:  %4 = bitcast i32 %3 to <2 x i16>

; Note: we can't optimize this as the allocated type size and loaded type sizes
; don't match. Maybe we could trunc %3 from i32 to i16? See CA-4382.

; CHECK: define spir_kernel void @__vecz_v4_load_type_size_mismatch_no_bitcast(ptr addrspace(1) %p)
; CHECK:  %data = alloca i32, align 4
; CHECK:  %4 = load i16, ptr %data, align 2

; Note: we can't optimize this as the allocated type size and loaded type sizes
; don't match. Maybe we could trunc %3 from i32 to i16? See CA-4382.

; CHECK: define spir_kernel void @__vecz_v4_store_type_size_mismatch_no_bitcast(ptr addrspace(1) %p)
; CHECK:  %data = alloca i32, align 4
; CHECK:  %4 = load i32, ptr %data, align 4
