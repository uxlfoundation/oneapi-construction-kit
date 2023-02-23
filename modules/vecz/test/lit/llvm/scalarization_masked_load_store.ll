; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -vecz-passes=scalarize -vecz-simd-width=4 -vecz-choices=FullScalarization -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)

declare <2 x float> @__vecz_b_masked_load4_Dv2_fPDv2_fDv2_b(<2 x float>*, <2 x i1>)
declare void @__vecz_b_masked_store4_Dv2_fPDv2_fDv2_b(<2 x float>, <2 x float>*, <2 x i1>)

define spir_kernel void @scalarize_masked_memops(<2 x float>* %pa, <2 x float>* %pz) {
entry:
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %head = insertelement <2 x i64> undef, i64 %idx, i64 0
  %splat = shufflevector <2 x i64> %head, <2 x i64> undef, <2 x i32> zeroinitializer
  %idxs = add <2 x i64> %splat, <i64 0, i64 1>
  %mask = icmp slt <2 x i64> %idxs, <i64 8, i64 8>
  %aptr = getelementptr <2 x float>, <2 x float>* %pa, i64 %idx
  %ld = call <2 x float> @__vecz_b_masked_load4_Dv2_fPDv2_fDv2_b(<2 x float>* %aptr, <2 x i1> %mask)
  %zptr = getelementptr <2 x float>, <2 x float>* %pz, i64 %idx
  call void @__vecz_b_masked_store4_Dv2_fPDv2_fDv2_b(<2 x float> %ld, <2 x float>* %zptr, <2 x i1> %mask)
  ret void
 ; CHECK:  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
 ; CHECK:  %[[IDXS0:.*]] = add i64 %idx, 0
 ; CHECK:  %[[IDXS1:.*]] = add i64 %idx, 1
 ; CHECK:  %[[MASK0:.*]] = icmp slt i64 %[[IDXS0]], 8
 ; CHECK:  %[[MASK1:.*]] = icmp slt i64 %[[IDXS1]], 8
 ; CHECK-GE15:  %aptr = getelementptr <2 x float>, ptr %pa, i64 %idx
 ; CHECK-LT15:  %aptr = getelementptr <2 x float>, <2 x float>* %pa, i64 %idx
 ; CHECK-GE15:  %[[TMP1:.*]] = getelementptr float, ptr %aptr, i32 0
 ; CHECK-LT15:  %[[TMP0:.*]] = bitcast <2 x float>* %aptr to float*
 ; CHECK-LT15:  %[[TMP1:.*]] = getelementptr float, float* %[[TMP0]], i32 0
 ; CHECK-GE15:  %[[TMP2:.*]] = getelementptr float, ptr %aptr, i32 1
 ; CHECK-LT15:  %[[TMP2:.*]] = getelementptr float, float* %[[TMP0]], i32 1
 ; CHECK-GE15:  %[[TMP3:.*]] = call float @__vecz_b_masked_load4_fu3ptrb(ptr %[[TMP1]], i1 %[[MASK0]])
 ; CHECK-LT15:  %[[TMP3:.*]] = call float @__vecz_b_masked_load4_fPfb(float* %[[TMP1]], i1 %[[MASK0]])
 ; CHECK-GE15:  %[[TMP4:.*]] = call float @__vecz_b_masked_load4_fu3ptrb(ptr %[[TMP2]], i1 %[[MASK1]])
 ; CHECK-LT15:  %[[TMP4:.*]] = call float @__vecz_b_masked_load4_fPfb(float* %[[TMP2]], i1 %[[MASK1]])
 ; CHECK-GE15:  %zptr = getelementptr <2 x float>, ptr %pz, i64 %idx
 ; CHECK-LT15:  %zptr = getelementptr <2 x float>, <2 x float>* %pz, i64 %idx
 ; CHECK-LT15:  %[[TMP5:.*]] = bitcast <2 x float>* %zptr to float*
 ; CHECK-GE15:  %[[TMP6:.*]] = getelementptr float, ptr %zptr, i32 0
 ; CHECK-LT15:  %[[TMP6:.*]] = getelementptr float, float* %[[TMP5]], i32 0
 ; CHECK-GE15:  %[[TMP7:.*]] = getelementptr float, ptr %zptr, i32 1
 ; CHECK-LT15:  %[[TMP7:.*]] = getelementptr float, float* %[[TMP5]], i32 1
 ; CHECK-GE15:  call void @__vecz_b_masked_store4_fu3ptrb(float %[[TMP3]], ptr %[[TMP6]], i1 %[[MASK0]])
 ; CHECK-LT15:  call void @__vecz_b_masked_store4_fPfb(float %[[TMP3]], float* %[[TMP6]], i1 %[[MASK0]])
 ; CHECK-GE15:  call void @__vecz_b_masked_store4_fu3ptrb(float %[[TMP4]], ptr %[[TMP7]], i1 %[[MASK1]])
 ; CHECK-LT15:  call void @__vecz_b_masked_store4_fPfb(float %[[TMP4]], float* %[[TMP7]], i1 %[[MASK1]])
 ; CHECK:  ret void

}
