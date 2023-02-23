; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -vecz-passes=remove-int-ptr -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define spir_kernel void @__vecz_v4_intptr_cast_i8(
; CHECK: %shl = shl i64 %call, 2
; CHECK-GE15: %remove_intptr = getelementptr i8, ptr addrspace(1) %in, i64 %shl
; CHECK-LT15: %remove_intptr = getelementptr i8, i8 addrspace(1)* %in, i64 %shl
; CHECK-GE15: %remove_intptr1 = ptrtoint ptr addrspace(1) %remove_intptr to i64
; CHECK-LT15: %remove_intptr1 = ptrtoint i8 addrspace(1)* %remove_intptr to i64
; CHECK-GE15: store i64 %remove_intptr1, ptr addrspace(1) %out, align 8
; CHECK-LT15: store i64 %remove_intptr1, i64 addrspace(1)* %out, align 8
define spir_kernel void @intptr_cast_i8(i8 addrspace(1)* %in, i64 addrspace(1)* %out) {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %0 = ptrtoint i8 addrspace(1)* %in to i64
  %shl = shl i64 %call, 2
  %add = add i64 %shl, %0
  store i64 %add, i64 addrspace(1)* %out, align 8
  ret void
}

; CHECK-LABEL: define spir_kernel void @__vecz_v4_intptr_cast_i16(
; CHECK: %shl = shl i64 %call, 2
; CHECK-GE15: %remove_intptr = getelementptr i8, ptr addrspace(1) %in, i64 %shl
; CHECK-LT15: %0 = bitcast i16 addrspace(1)* %in to i8 addrspace(1)
; CHECK-LT15: %remove_intptr = getelementptr i8, i8 addrspace(1)* %0, i64 %shl
; CHECK-GE15: %remove_intptr1 = ptrtoint ptr addrspace(1) %remove_intptr to i64
; CHECK-LT15: %remove_intptr1 = ptrtoint i8 addrspace(1)* %remove_intptr to i64
; CHECK-GE15: store i64 %remove_intptr1, ptr addrspace(1) %out, align 8
; CHECK-LT15: store i64 %remove_intptr1, i64 addrspace(1)* %out, align 8
define spir_kernel void @intptr_cast_i16(i16 addrspace(1)* %in, i64 addrspace(1)* %out) {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %0 = ptrtoint i16 addrspace(1)* %in to i64
  %shl = shl i64 %call, 2
  %add = add i64 %shl, %0
  store i64 %add, i64 addrspace(1)* %out, align 8
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)
