; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc --vecz-passes=interleave-combine-loads -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

; This test checks that we can optimize interleaved accesses out of order.

define dso_local spir_kernel void @interleaved_load_4(i32 addrspace(1)* %out, i32 addrspace(1)* %in, i32 %stride) {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %conv = trunc i64 %call to i32
  %call1 = tail call spir_func i64 @_Z13get_global_idj(i32 1)
  %conv2 = trunc i64 %call1 to i32
  %mul = mul nsw i32 %conv2, %stride
  %add = add nsw i32 %conv, %mul
  %mul3 = shl nsw i32 %add, 1
  %add4 = or i32 %mul3, 1
  %idxprom = sext i32 %add4 to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 %idxprom
  %0 = call <4 x i32> @__vecz_b_interleaved_load4_2_Dv4_jPU3AS1j(i32 addrspace(1)* %arrayidx)
  %idxprom8 = sext i32 %mul3 to i64
  %arrayidx9 = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 %idxprom8
  %1 = call <4 x i32> @__vecz_b_interleaved_load4_2_Dv4_jPU3AS1j(i32 addrspace(1)* %arrayidx9)
  %sub1 = sub nsw <4 x i32> %0, %1
  %idxprom12 = sext i32 %add to i64
  %arrayidx13 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %idxprom12
  %2 = bitcast i32 addrspace(1)* %arrayidx13 to <4 x i32> addrspace(1)*
  store <4 x i32> %sub1, <4 x i32> addrspace(1)* %2, align 4
  ret void
}

; CHECK: __vecz_v4_interleaved_load_4(
; CHECK-LT15:  [[TMP0:%.*]] = bitcast i32 addrspace(1)* [[PTR:%.*]] to <4 x i32> addrspace(1)*
; CHECK-GE15:  [[TMP1:%.*]] = load <4 x i32>, ptr addrspace(1) [[PTR:%.*]], align 4
; CHECK-LT15:  [[TMP1:%.*]] = load <4 x i32>, <4 x i32> addrspace(1)* [[TMP0]], align 4
; CHECK-GE15:  [[TMP2:%.*]] = getelementptr i32, ptr addrspace(1) [[PTR]], i32 4
; CHECK-LT15:  [[TMP2:%.*]] = getelementptr i32, i32 addrspace(1)* [[PTR]], i32 4
; CHECK-LT15:  [[TMP3:%.*]] = bitcast i32 addrspace(1)* [[TMP2]] to <4 x i32> addrspace(1)*
; CHECK-GE15:  [[TMP4:%.*]] = load <4 x i32>, ptr addrspace(1) [[TMP2]], align 4
; CHECK-LT15:  [[TMP4:%.*]] = load <4 x i32>, <4 x i32> addrspace(1)* [[TMP3]], align 4
; CHECK:  %deinterleave = shufflevector <4 x i32> [[TMP1]], <4 x i32> [[TMP4]], <4 x i32> <i32 0, i32 2, i32 4, i32 6>
; CHECK:  %deinterleave1 = shufflevector <4 x i32> [[TMP1]], <4 x i32> [[TMP4]], <4 x i32> <i32 1, i32 3, i32 5, i32 7>
; CHECK:  %sub1 = sub nsw <4 x i32> %deinterleave1, %deinterleave


declare spir_func i64 @_Z13get_global_idj(i32)
declare <4 x i32> @__vecz_b_interleaved_load4_2_Dv4_jPU3AS1j(i32 addrspace(1)*)
