; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test_calls -vecz-passes=packetizer -vecz-simd-width=16 -vecz-choices=TargetIndependentPacketization -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @test_calls(i8* %pa, i8* %pb, i8* %pd) {
entry:
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %a = getelementptr i8, i8* %pa, i64 %idx
  %b = getelementptr i8, i8* %pb, i64 %idx
  %d = getelementptr i8, i8* %pd, i64 %idx
  %la = load i8, i8* %a, align 16
  %lb = load i8, i8* %b, align 16
  %res = tail call i8 @llvm.fshl.i8(i8 %la, i8 %lb, i8 4)
  store i8 %res, i8* %d, align 16
  ret void
}

declare i8 @llvm.fshl.i8(i8, i8, i8)

; CHECK-GE15: define spir_kernel void @__vecz_v16_test_calls(ptr %pa, ptr %pb, ptr %pd)
; CHECK-LT15: define spir_kernel void @__vecz_v16_test_calls(i8* %pa, i8* %pb, i8* %pd)
; CHECK: entry:

; It checks that the fshl intrinsic of i8 gets widened by a factor of 16
; CHECK-GE15: %[[LDA:.+]] = load <16 x i8>, ptr %{{.+}}
; CHECK-LT15: %[[LDA:.+]] = load <16 x i8>, <16 x i8>* %{{.+}}
; CHECK-GE15: %[[LDB:.+]] = load <16 x i8>, ptr %{{.+}}
; CHECK-LT15: %[[LDB:.+]] = load <16 x i8>, <16 x i8>* %{{.+}}
; CHECK: %[[RES:.+]] = call <16 x i8> @llvm.fshl.v16i8(<16 x i8> %[[LDA]], <16 x i8> %[[LDB]], <16 x i8> <{{(i8 4, )+i8 4}}>)
; CHECK-GE15: store <16 x i8> %[[RES]], ptr %{{.+}}
; CHECK-LT15: store <16 x i8> %[[RES]], <16 x i8>* %{{.+}}

; CHECK: ret void
