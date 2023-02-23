; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %muxc --passes replace-printf,verify -S %s | %filecheck %t

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

@.str = private unnamed_addr addrspace(2) constant [10 x i8] c"id = %lu\0A\00", align 1

; CHECK-LABEL: define spir_kernel void @do_printf(
; CHECK: %id = call spir_func i64 @_Z13get_global_idj(i32 0)
; CHECK-GE15: = call spir_func i32 @0(ptr addrspace(1) %0, i64 %id)
; CHECK-LT15: = call spir_func i32 @0(i8 addrspace(1)* %0, i64 %id)
define spir_kernel void @do_printf(i32 addrspace(1)* %a) {
entry:
  %id = call spir_func i64 @_Z13get_global_idj(i32 0)
  %call1 = tail call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* getelementptr inbounds ([10 x i8], [10 x i8] addrspace(2)* @.str, i64 0, i64 0), i64 %id)
  ret void
}

; CHECK-LABEL: define linkonce_odr spir_func i32 @0(
; CHECK-LABEL: store:
; Store the printf call ID
; CHECK-GE15: [[ID_PTR:%.*]] = getelementptr i8, ptr addrspace(1) [[PTR:%.*]], i32 [[IDX:%.*]]
; CHECK-GE15: store i32 0, ptr addrspace(1) [[ID_PTR]]

; CHECK-LT15: [[ID_PTR:%.*]] = getelementptr i8, i8 addrspace(1)* [[PTR:%.*]], i32 [[IDX:%.*]]
; CHECK-LT15: [[ID_PTR2:%.*]] = bitcast i8 addrspace(1)* [[ID_PTR]] to i32 addrspace(1)*
; CHECK-LT15: store i32 0, i32 addrspace(1)* [[ID_PTR2]]

; Store the argument we're printing at the next address
; CHECK: [[NEXT_IDX:%.*]] = add i32 [[IDX]], 4
; CHECK-GE15: [[VAL_PTR:%.*]] = getelementptr i8, ptr addrspace(1) [[PTR]], i32 [[NEXT_IDX]]
; CHECK-GE15: store i64 %1, ptr addrspace(1) [[VAL_PTR]]
; CHECK-LT15: [[VAL_PTR:%.*]] = getelementptr i8, i8 addrspace(1)* [[PTR]], i32 [[NEXT_IDX]]
; CHECK-LT15: [[VAL_PTR2:%.*]] = bitcast i8 addrspace(1)* [[VAL_PTR]] to i64 addrspace(1)*
; CHECK-LT15: store i64 %1, i64 addrspace(1)* [[VAL_PTR2]]

declare spir_func i64 @_Z13get_global_idj(i32)

declare spir_func i32 @printf(i8 addrspace(2)*, ...)
