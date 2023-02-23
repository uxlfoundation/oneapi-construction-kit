; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %llvm-as -o %t.bc %s
; RUN: %veczc -o %t.vecz.bc %t.bc -k main -w 4
;
; This test contains no CHECK directives as success is indicated simply by
; successful vectorization through veczc. A non-zero return code indicates
; failure and so the test will fail.

; ModuleID = 'getDimensions.bc'
source_filename = "getDimensions.dxil"

target triple = "dxil-ms-dx"

%dx.types.Handle     = type { i8* }
%dx.types.Dimensions = type { i32, i32, i32, i32 }

define void @main(i8 addrspace(0)* %ptr) {
  %id = call i32 @dx.op.flattenedThreadIdInGroup.i32(i32 96)
  %gep = getelementptr i8, i8 addrspace(0)* %ptr, i32 %id
  %x = load i8, i8 addrspace(0)* %gep
  %handle = insertvalue %dx.types.Handle undef, i8 addrspace(0)* %gep, 0
  %dim = call %dx.types.Dimensions @dx.op.getDimensions(i32 72, %dx.types.Handle %handle, i32 0)
  ret void
}

declare i32 @dx.op.flattenedThreadIdInGroup.i32(i32) #1

declare %dx.types.Dimensions @dx.op.getDimensions(i32, %dx.types.Handle, i32)
