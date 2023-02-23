; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test_calls -vecz-passes=scalarize -vecz-simd-width=4 -vecz-choices=FullScalarization -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @test_calls(<4 x float>* %pa, <4 x float>* %pb, <4 x i32>* %pc, <4 x float>* %pd) {
entry:
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %a = getelementptr <4 x float>, <4 x float>* %pa, i64 %idx
  %b = getelementptr <4 x float>, <4 x float>* %pb, i64 %idx
  %c = getelementptr <4 x i32>, <4 x i32>* %pc, i64 %idx
  %d = getelementptr <4 x float>, <4 x float>* %pd, i64 %idx
  %0 = load <4 x float>, <4 x float>* %a, align 16
  %1 = load <4 x float>, <4 x float>* %b, align 16
  %2 = load <4 x i32>, <4 x i32>* %c, align 16
  %call = call spir_func <4 x float> @_Z14convert_float4Dv4_i(<4 x i32> %2)
  %3 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %0, <4 x float> %1, <4 x float> %call)
  store <4 x float> %3, <4 x float>* %d, align 16
  ret void
}

declare spir_func <4 x float> @_Z14convert_float4Dv4_i(<4 x i32>)
declare spir_func float @_Z13convert_floati(i32)
declare <4x float> @llvm.fmuladd.v4f32(<4 x float>, <4 x float>, <4 x float>)

; CHECK-GE15: define spir_kernel void @__vecz_v4_test_calls(ptr %pa, ptr %pb, ptr %pc, ptr %pd)
; CHECK-LT15: define spir_kernel void @__vecz_v4_test_calls(<4 x float>* %pa, <4 x float>* %pb, <4 x i32>* %pc, <4 x float>* %pd)
; CHECK: entry:
; CHECK-LT15: %[[A:.+]] = bitcast <4 x float>* %a to float*
; CHECK-GE15: %[[A_0:.+]] = getelementptr float, ptr %a, i32 0
; CHECK-GE15: %[[A_1:.+]] = getelementptr float, ptr %a, i32 1
; CHECK-GE15: %[[A_2:.+]] = getelementptr float, ptr %a, i32 2
; CHECK-GE15: %[[A_3:.+]] = getelementptr float, ptr %a, i32 3
; CHECK-LT15: %[[A_0:.+]] = getelementptr float, float* %[[A]], i32 0
; CHECK-LT15: %[[A_1:.+]] = getelementptr float, float* %[[A]], i32 1
; CHECK-LT15: %[[A_2:.+]] = getelementptr float, float* %[[A]], i32 2
; CHECK-LT15: %[[A_3:.+]] = getelementptr float, float* %[[A]], i32 3
; CHECK-GE15: %[[LA_0:.+]] = load float, ptr %[[A_0]]
; CHECK-LT15: %[[LA_0:.+]] = load float, float* %[[A_0]]
; CHECK-GE15: %[[LA_1:.+]] = load float, ptr %[[A_1]]
; CHECK-LT15: %[[LA_1:.+]] = load float, float* %[[A_1]]
; CHECK-GE15: %[[LA_2:.+]] = load float, ptr %[[A_2]]
; CHECK-LT15: %[[LA_2:.+]] = load float, float* %[[A_2]]
; CHECK-GE15: %[[LA_3:.+]] = load float, ptr %[[A_3]]
; CHECK-LT15: %[[LA_3:.+]] = load float, float* %[[A_3]]
; CHECK-GE15: %[[B_0:.+]] = getelementptr float, ptr %b, i32 0
; CHECK-GE15: %[[B_1:.+]] = getelementptr float, ptr %b, i32 1
; CHECK-GE15: %[[B_2:.+]] = getelementptr float, ptr %b, i32 2
; CHECK-GE15: %[[B_3:.+]] = getelementptr float, ptr %b, i32 3
; CHECK-LT15: %[[B:.+]] = bitcast <4 x float>* %b to float*
; CHECK-LT15: %[[B_0:.+]] = getelementptr float, float* %[[B]], i32 0
; CHECK-LT15: %[[B_1:.+]] = getelementptr float, float* %[[B]], i32 1
; CHECK-LT15: %[[B_2:.+]] = getelementptr float, float* %[[B]], i32 2
; CHECK-LT15: %[[B_3:.+]] = getelementptr float, float* %[[B]], i32 3
; CHECK-GE15: %[[LB_0:.+]] = load float, ptr %[[B_0]]
; CHECK-LT15: %[[LB_0:.+]] = load float, float* %[[B_0]]
; CHECK-GE15: %[[LB_1:.+]] = load float, ptr %[[B_1]]
; CHECK-LT15: %[[LB_1:.+]] = load float, float* %[[B_1]]
; CHECK-GE15: %[[LB_2:.+]] = load float, ptr %[[B_2]]
; CHECK-LT15: %[[LB_2:.+]] = load float, float* %[[B_2]]
; CHECK-GE15: %[[LB_3:.+]] = load float, ptr %[[B_3]]
; CHECK-LT15: %[[LB_3:.+]] = load float, float* %[[B_3]]
; CHECK-GE15: %[[C_0:.+]] = getelementptr i32, ptr %c, i32 0
; CHECK-GE15: %[[C_1:.+]] = getelementptr i32, ptr %c, i32 1
; CHECK-GE15: %[[C_2:.+]] = getelementptr i32, ptr %c, i32 2
; CHECK-GE15: %[[C_3:.+]] = getelementptr i32, ptr %c, i32 3
; CHECK-LT15: %[[C:.+]] = bitcast <4 x i32>* %c to i32*
; CHECK-LT15: %[[C_0:.+]] = getelementptr i32, i32* %[[C]], i32 0
; CHECK-LT15: %[[C_1:.+]] = getelementptr i32, i32* %[[C]], i32 1
; CHECK-LT15: %[[C_2:.+]] = getelementptr i32, i32* %[[C]], i32 2
; CHECK-LT15: %[[C_3:.+]] = getelementptr i32, i32* %[[C]], i32 3
; CHECK-GE15: %[[LC_0:.+]] = load i32, ptr %[[C_0]]
; CHECK-LT15: %[[LC_0:.+]] = load i32, i32* %[[C_0]]
; CHECK-GE15: %[[LC_1:.+]] = load i32, ptr %[[C_1]]
; CHECK-LT15: %[[LC_1:.+]] = load i32, i32* %[[C_1]]
; CHECK-GE15: %[[LC_2:.+]] = load i32, ptr %[[C_2]]
; CHECK-LT15: %[[LC_2:.+]] = load i32, i32* %[[C_2]]
; CHECK-GE15: %[[LC_3:.+]] = load i32, ptr %[[C_3]]
; CHECK-LT15: %[[LC_3:.+]] = load i32, i32* %[[C_3]]
; CHECK: %[[CALL1:.+]] = call spir_func float @_Z13convert_floati(i32 %[[LC_0]])
; CHECK: %[[CALL2:.+]] = call spir_func float @_Z13convert_floati(i32 %[[LC_1]])
; CHECK: %[[CALL3:.+]] = call spir_func float @_Z13convert_floati(i32 %[[LC_2]])
; CHECK: %[[CALL4:.+]] = call spir_func float @_Z13convert_floati(i32 %[[LC_3]])
; CHECK: %[[FMAD_0:.+]] = call float @llvm.fmuladd.f32(float %[[LA_0]], float %[[LB_0]], float %[[CALL1]])
; CHECK: %[[FMAD_1:.+]] = call float @llvm.fmuladd.f32(float %[[LA_1]], float %[[LB_1]], float %[[CALL2]])
; CHECK: %[[FMAD_2:.+]] = call float @llvm.fmuladd.f32(float %[[LA_2]], float %[[LB_2]], float %[[CALL3]])
; CHECK: %[[FMAD_3:.+]] = call float @llvm.fmuladd.f32(float %[[LA_3]], float %[[LB_3]], float %[[CALL4]])
; CHECK-GE15: %[[D_0:.+]] = getelementptr float, ptr %d, i32 0
; CHECK-GE15: %[[D_1:.+]] = getelementptr float, ptr %d, i32 1
; CHECK-GE15: %[[D_2:.+]] = getelementptr float, ptr %d, i32 2
; CHECK-GE15: %[[D_3:.+]] = getelementptr float, ptr %d, i32 3
; CHECK-LT15: %[[D:.+]] = bitcast <4 x float>* %d to float*
; CHECK-LT15: %[[D_0:.+]] = getelementptr float, float* %[[D]], i32 0
; CHECK-LT15: %[[D_1:.+]] = getelementptr float, float* %[[D]], i32 1
; CHECK-LT15: %[[D_2:.+]] = getelementptr float, float* %[[D]], i32 2
; CHECK-LT15: %[[D_3:.+]] = getelementptr float, float* %[[D]], i32 3
; CHECK-GE15: store float %[[FMAD_0]], ptr %[[D_0]]
; CHECK-LT15: store float %[[FMAD_0]], float* %[[D_0]]
; CHECK-GE15: store float %[[FMAD_1]], ptr %[[D_1]]
; CHECK-LT15: store float %[[FMAD_1]], float* %[[D_1]]
; CHECK-GE15: store float %[[FMAD_2]], ptr %[[D_2]]
; CHECK-LT15: store float %[[FMAD_2]], float* %[[D_2]]
; CHECK-GE15: store float %[[FMAD_3]], ptr %[[D_3]]
; CHECK-LT15: store float %[[FMAD_3]], float* %[[D_3]]
; CHECK: ret void
