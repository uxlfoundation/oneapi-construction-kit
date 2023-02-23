; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test_calls -vecz-passes=packetizer -vecz-simd-width=8 -vecz-choices=TargetIndependentPacketization -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @test_calls(<4 x float>* %pa, <4 x float>* %pb, <4 x float>* %pc, <4 x float>* %pd) {
entry:
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %a = getelementptr <4 x float>, <4 x float>* %pa, i64 %idx
  %b = getelementptr <4 x float>, <4 x float>* %pb, i64 %idx
  %c = getelementptr <4 x float>, <4 x float>* %pc, i64 %idx
  %d = getelementptr <4 x float>, <4 x float>* %pd, i64 %idx
  %la = load <4 x float>, <4 x float>* %a, align 16
  %lb = load <4 x float>, <4 x float>* %b, align 16
  %lc = load <4 x float>, <4 x float>* %c, align 16
  br label %loop

loop:
  %n = phi i32 [ %dec, %loop ], [ 10, %entry ]
  %acc = phi <4 x float> [ %fma, %loop ], [ %la, %entry ]
  %fma = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %acc, <4 x float> %lb, <4 x float> %lc)
  %dec = sub i32 %n, 1
  %cmp = icmp ne i32 %dec, 0
  br i1 %cmp, label %loop, label %end

end:
  store <4 x float> %fma, <4 x float>* %d, align 16
  ret void
}

declare <4x float> @llvm.fmuladd.v4f32(<4 x float>, <4 x float>, <4 x float>)

; CHECK-GE15: define spir_kernel void @__vecz_v8_test_calls(ptr %pa, ptr %pb, ptr %pc, ptr %pd)
; CHECK-LT15: define spir_kernel void @__vecz_v8_test_calls(<4 x float>* %pa, <4 x float>* %pb, <4 x float>* %pc, <4 x float>* %pd)
; CHECK: entry:

; It checks that the fmuladd intrinsic of <4 x float> gets widened by a factor of 8,
; to produce a PAIR of <16 x float>s.
; CHECK-GE15: %[[LDA0:.+]] = load <16 x float>, ptr %{{.+}}, align 4
; CHECK-LT15: %[[LDA0:.+]] = load <16 x float>, <16 x float>* %{{.+}}, align 4
; CHECK-GE15: %[[LDA1:.+]] = load <16 x float>, ptr %{{.+}}, align 4
; CHECK-LT15: %[[LDA1:.+]] = load <16 x float>, <16 x float>* %{{.+}}, align 4
; CHECK-GE15: %[[LDB0:.+]] = load <16 x float>, ptr %{{.+}}, align 4
; CHECK-LT15: %[[LDB0:.+]] = load <16 x float>, <16 x float>* %{{.+}}, align 4
; CHECK-GE15: %[[LDB1:.+]] = load <16 x float>, ptr %{{.+}}, align 4
; CHECK-LT15: %[[LDB1:.+]] = load <16 x float>, <16 x float>* %{{.+}}, align 4
; CHECK-GE15: %[[LDC0:.+]] = load <16 x float>, ptr %{{.+}}, align 4
; CHECK-LT15: %[[LDC0:.+]] = load <16 x float>, <16 x float>* %{{.+}}, align 4
; CHECK-GE15: %[[LDC1:.+]] = load <16 x float>, ptr %{{.+}}, align 4
; CHECK-LT15: %[[LDC1:.+]] = load <16 x float>, <16 x float>* %{{.+}}, align 4

; CHECK: loop:
; CHECK: %[[ACC0:.+]] = phi <16 x float> [ %[[FMA0:.+]], %loop ], [ %[[LDA0]], %entry ]
; CHECK: %[[ACC1:.+]] = phi <16 x float> [ %[[FMA1:.+]], %loop ], [ %[[LDA1]], %entry ]

; CHECK: %[[FMA0]] = call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %[[ACC0]], <16 x float> %[[LDB0]], <16 x float> %[[LDC0]])
; CHECK: %[[FMA1]] = call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %[[ACC1]], <16 x float> %[[LDB1]], <16 x float> %[[LDC1]])

; CHECK: end:
; CHECK-GE15: store <16 x float> %[[FMA0]], ptr %{{.+}}, align 16
; CHECK-LT15: store <16 x float> %[[FMA0]], <16 x float>* %{{.+}}, align 16
; CHECK-GE15: store <16 x float> %[[FMA1]], ptr %{{.+}}, align 16
; CHECK-LT15: store <16 x float> %[[FMA1]], <16 x float>* %{{.+}}, align 16

; CHECK: ret void
