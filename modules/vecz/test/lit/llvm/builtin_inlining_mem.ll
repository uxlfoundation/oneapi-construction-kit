
; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -vecz-passes=builtin-inlining,verify -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

; FIXME: CA-4331 - we can't inline non-i8 memcpy/memset

define spir_kernel void @test_memset_i16(i64* %z) {
  %dst = bitcast i64* %z to i16*
  call void @llvm.memset.p0i16.i64(i16* %dst, i8 42, i64 18, i32 8, i1 false)
  ret void
}

; CHECK-LABEL-LT15: define spir_kernel void @__vecz_v4_test_memset_i16(i64* %z)
; CHECK-LT15: call void @llvm.memset.p0i16.i64(i16* align 8 %dst, i8 42, i64 18, i1 false)

; CHECK-LABEL-GE15: define spir_kernel void @__vecz_v4_test_memset_i16(ptr %z)
; CHECK-GE15: [[D1:%.*]] = getelementptr inbounds i8, ptr %dst, i64 0
; CHECK-GE15: store i64 3038287259199220266, ptr [[D1]], align 8

; CHECK-GE15: [[D2:%.*]] = getelementptr inbounds i8, ptr %dst, i64 8
; CHECK-GE15: store i64 3038287259199220266, ptr [[D2]], align 8

; CHECK-GE15: [[D3:%.*]] = getelementptr inbounds i8, ptr %dst, i64 16
; CHECK-GE15: store i8 42, ptr [[D3]], align 1

; CHECK-GE15: [[D4:%.*]] = getelementptr inbounds i8, ptr %dst, i64 17
; CHECK-GE15: store i8 42, ptr [[D4]], align 1
; CHECK: }

define spir_kernel void @test_memcpy_i16(i64* %a, i64* %z) {
  %src = bitcast i64* %a to i16*
  %dst = bitcast i64* %z to i16*
  call void @llvm.memcpy.p0i16.p0i16.i64(i16* %dst, i16* %src, i64 18, i32 8, i1 false)
  ret void
}

; CHECK-LABEL-LT15: define spir_kernel void @__vecz_v4_test_memcpy_i16(i64* %a, i64* %z)
; CHECK-LT15: call void @llvm.memcpy.p0i16.p0i16.i64(i16* align 8 %dst, i16* align 8 %src, i64 18, i1 false)

; CHECK-LABEL-GE15: define spir_kernel void @__vecz_v4_test_memcpy_i16(ptr %a, ptr %z)
; CHECK-GE15: [[S1:%.*]] = getelementptr inbounds i8, ptr %src, i64 0
; CHECK-GE15: [[D1:%.*]] = getelementptr inbounds i8, ptr %dst, i64 0
; CHECK-GE15: [[SRC1:%.*]] = load i64, ptr [[S1]], align 8
; CHECK-GE15: store i64 [[SRC1]], ptr [[D1]], align 8

; CHECK-GE15: [[S2:%.*]] = getelementptr inbounds i8, ptr %src, i64 8
; CHECK-GE15: [[D2:%.*]] = getelementptr inbounds i8, ptr %dst, i64 8
; CHECK-GE15: [[SRC2:%.*]] = load i64, ptr [[S2]], align 8
; CHECK-GE15: store i64 [[SRC2]], ptr [[D2]], align 8

; CHECK-GE15: [[S3:%.*]] = getelementptr inbounds i8, ptr %src, i64 16
; CHECK-GE15: [[D3:%.*]] = getelementptr inbounds i8, ptr %dst, i64 16
; CHECK-GE15: [[SRC3:%.*]] = load i8, ptr [[S3]], align 1
; CHECK-GE15: store i8 [[SRC3]], ptr [[D3]], align 1

; CHECK-GE15: [[S4:%.*]] = getelementptr inbounds i8, ptr %src, i64 17
; CHECK-GE15: [[D4:%.*]] = getelementptr inbounds i8, ptr %dst, i64 17
; CHECK-GE15: [[SRC4:%.*]] = load i8, ptr [[S4]], align 1
; CHECK-GE15: store i8 [[SRC4]], ptr [[D4]], align 1
; CHECK: }

declare void @llvm.memset.p0i16.i64(i16*, i8, i64, i32, i1)
declare void @llvm.memcpy.p0i16.p0i16.i64(i16*, i16*, i64, i32, i1)
