
; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -vecz-passes=builtin-inlining,verify -vecz-simd-width=4 -S < %s | %filecheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

; FIXME: CA-4331 - we can't inline non-i8 memcpy/memset

define spir_kernel void @test_memset_i16(i64* %z) {
  %dst = bitcast i64* %z to i16*
  call void @llvm.memset.p0i16.i64(i16* %dst, i8 42, i64 18, i32 8, i1 false)
  ret void
}


; CHECK-LABEL: define spir_kernel void @__vecz_v4_test_memset_i16(ptr %z)
; CHECK: [[D1:%.*]] = getelementptr inbounds i8, ptr %dst, i64 0
; CHECK: store i64 3038287259199220266, ptr [[D1]], align 8

; CHECK: [[D2:%.*]] = getelementptr inbounds i8, ptr %dst, i64 8
; CHECK: store i64 3038287259199220266, ptr [[D2]], align 8

; CHECK: [[D3:%.*]] = getelementptr inbounds i8, ptr %dst, i64 16
; CHECK: store i8 42, ptr [[D3]], align 1

; CHECK: [[D4:%.*]] = getelementptr inbounds i8, ptr %dst, i64 17
; CHECK: store i8 42, ptr [[D4]], align 1
; CHECK: }

define spir_kernel void @test_memcpy_i16(i64* %a, i64* %z) {
  %src = bitcast i64* %a to i16*
  %dst = bitcast i64* %z to i16*
  call void @llvm.memcpy.p0i16.p0i16.i64(i16* %dst, i16* %src, i64 18, i32 8, i1 false)
  ret void
}


; CHECK-LABEL: define spir_kernel void @__vecz_v4_test_memcpy_i16(ptr %a, ptr %z)
; CHECK: [[S1:%.*]] = getelementptr inbounds i8, ptr %src, i64 0
; CHECK: [[D1:%.*]] = getelementptr inbounds i8, ptr %dst, i64 0
; CHECK: [[SRC1:%.*]] = load i64, ptr [[S1]], align 8
; CHECK: store i64 [[SRC1]], ptr [[D1]], align 8

; CHECK: [[S2:%.*]] = getelementptr inbounds i8, ptr %src, i64 8
; CHECK: [[D2:%.*]] = getelementptr inbounds i8, ptr %dst, i64 8
; CHECK: [[SRC2:%.*]] = load i64, ptr [[S2]], align 8
; CHECK: store i64 [[SRC2]], ptr [[D2]], align 8

; CHECK: [[S3:%.*]] = getelementptr inbounds i8, ptr %src, i64 16
; CHECK: [[D3:%.*]] = getelementptr inbounds i8, ptr %dst, i64 16
; CHECK: [[SRC3:%.*]] = load i8, ptr [[S3]], align 1
; CHECK: store i8 [[SRC3]], ptr [[D3]], align 1

; CHECK: [[S4:%.*]] = getelementptr inbounds i8, ptr %src, i64 17
; CHECK: [[D4:%.*]] = getelementptr inbounds i8, ptr %dst, i64 17
; CHECK: [[SRC4:%.*]] = load i8, ptr [[S4]], align 1
; CHECK: store i8 [[SRC4]], ptr [[D4]], align 1
; CHECK: }

declare void @llvm.memset.p0i16.i64(i16*, i8, i64, i32, i1)
declare void @llvm.memcpy.p0i16.p0i16.i64(i16*, i16*, i64, i32, i1)
