; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %llvm-as -o %t.bc %s
; RUN: %veczc -o %t.vecz.bc %t.bc -k main -w 4

; ModuleID = 'threadIdCall.bc'
source_filename = "threadIdCall.dxil"

target triple = "dxil-ms-dx"

%dx.types.Handle = type { i8* }
%dx.types.CBufRet.i32 = type { i32, i32, i32, i32 }
%dx.types.twoi32 = type { i32, i32 }

@TGSM0 = internal addrspace(3) global [2048 x i8] undef, align 4
@llvm.used = appending global [1 x i8*] [i8* addrspacecast (i8 addrspace(3)* getelementptr inbounds ([2048 x i8], [2048 x i8] addrspace(3)* @TGSM0, i32 0, i32 0) to i8*)], section "llvm.metadata"

define void @main() {
entry:
  %0 = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 2, i32 0, i32 0, i1 false)
  %1 = call i32 @dx.op.flattenedThreadIdInGroup.i32(i32 96)
  %2 = mul i32 %1, 4
  %3 = add i32 %2, 0
  %4 = getelementptr [2048 x i8], [2048 x i8] addrspace(3)* @TGSM0, i32 0, i32 %3
  %5 = bitcast i8 addrspace(3)* %4 to float addrspace(3)*
  store float 0.000000e+00, float addrspace(3)* %5, align 4
  %6 = call i32 @dx.op.groupId.i32(i32 94, i32 0)
  %7 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %0, i32 5)
  %8 = extractvalue %dx.types.CBufRet.i32 %7, 3
  %9 = add i32 %6, %8
  %10 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %0, i32 5)
  %11 = extractvalue %dx.types.CBufRet.i32 %10, 2
  %12 = call %dx.types.twoi32 @dx.op.binaryWithTwoOuts.i32(i32 43, i32 %9, i32 %11)
  %13 = extractvalue %dx.types.twoi32 %12, 1
  %14 = call i32 @dx.op.quaternary.i32(i32 53, i32 22, i32 10, i32 %13, i32 %1)
  ret void
}

; Function Attrs: nounwind readonly
declare %dx.types.Handle @dx.op.createHandle(i32, i8, i32, i32, i1) #0

; Function Attrs: nounwind readnone
declare i32 @dx.op.flattenedThreadIdInGroup.i32(i32) #1

; Function Attrs: nounwind readnone
declare i32 @dx.op.groupId.i32(i32, i32) #1

; Function Attrs: nounwind readonly
declare %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32, %dx.types.Handle, i32) #0

; Function Attrs: nounwind readnone
declare %dx.types.twoi32 @dx.op.binaryWithTwoOuts.i32(i32, i32, i32) #1

; Function Attrs: nounwind readnone
declare i32 @dx.op.quaternary.i32(i32, i32, i32, i32, i32) #1
