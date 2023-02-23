; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k widen_binops -vecz-passes=packetizer -vecz-simd-width=8 -vecz-choices=TargetIndependentPacketization -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @widen_binops(<4 x i32>* %pa, <4 x i32>* %pb, <4 x i64>* %pd) {
entry:
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %a = getelementptr <4 x i32>, <4 x i32>* %pa, i64 %idx
  %b = getelementptr <4 x i32>, <4 x i32>* %pb, i64 %idx
  %d = getelementptr <4 x i64>, <4 x i64>* %pd, i64 %idx
  %la = load <4 x i32>, <4 x i32>* %a, align 16
  %lb = load <4 x i32>, <4 x i32>* %b, align 16
  %xa = zext <4 x i32> %la to <4 x i64>
  %xb = zext <4 x i32> %lb to <4 x i64>
  %add = add nuw nsw <4 x i64> %xa, %xb
  store <4 x i64> %add, <4 x i64>* %d, align 16
  ret void
}

; CHECK-GE15: define spir_kernel void @__vecz_v8_widen_binops(ptr %pa, ptr %pb, ptr %pd)
; CHECK-LT15: define spir_kernel void @__vecz_v8_widen_binops(<4 x i32>* %pa, <4 x i32>* %pb, <4 x i64>* %pd)
; CHECK: entry:

; It checks that the zexts and add of <4 x i32> gets widened by a factor of 8,
; to produce PAIRs of <16 x i32>s.
; CHECK-GE15: %[[LDA0:.+]] = load <16 x i32>, ptr %{{.+}}, align 4
; CHECK-LT15: %[[LDA0:.+]] = load <16 x i32>, <16 x i32>* %{{.+}}, align 4
; CHECK-GE15: %[[LDA1:.+]] = load <16 x i32>, ptr %{{.+}}, align 4
; CHECK-LT15: %[[LDA1:.+]] = load <16 x i32>, <16 x i32>* %{{.+}}, align 4
; CHECK-GE15: %[[LDB0:.+]] = load <16 x i32>, ptr %{{.+}}, align 4
; CHECK-LT15: %[[LDB0:.+]] = load <16 x i32>, <16 x i32>* %{{.+}}, align 4
; CHECK-GE15: %[[LDB1:.+]] = load <16 x i32>, ptr %{{.+}}, align 4
; CHECK-LT15: %[[LDB1:.+]] = load <16 x i32>, <16 x i32>* %{{.+}}, align 4
; CHECK: %[[XA0:.+]] = zext <16 x i32> %[[LDA0]] to <16 x i64>
; CHECK: %[[XA1:.+]] = zext <16 x i32> %[[LDA1]] to <16 x i64>
; CHECK: %[[XB0:.+]] = zext <16 x i32> %[[LDB0]] to <16 x i64>
; CHECK: %[[XB1:.+]] = zext <16 x i32> %[[LDB1]] to <16 x i64>
; CHECK: %[[ADD0:.+]] = add nuw nsw <16 x i64> %[[XA0]], %[[XB0]]
; CHECK: %[[ADD1:.+]] = add nuw nsw <16 x i64> %[[XA1]], %[[XB1]]
; CHECK-GE15: store <16 x i64> %[[ADD0]], ptr %{{.+}}
; CHECK-LT15: store <16 x i64> %[[ADD0]], <16 x i64>* %{{.+}}
; CHECK-GE15: store <16 x i64> %[[ADD1]], ptr %{{.+}}
; CHECK-LT15: store <16 x i64> %[[ADD1]], <16 x i64>* %{{.+}}

; CHECK: ret void
