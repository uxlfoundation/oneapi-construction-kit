; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test_branch -vecz-passes=cfg-convert,packetizer -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_branch(i32 %a, i32* %b) {
entry:
  %conv = sext i32 %a to i64
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %cmp = icmp eq i64 %conv, %call
  br i1 %cmp, label %if.then, label %if.else

if.then:
  %idxprom = sext i32 %a to i64
  %arrayidx = getelementptr inbounds i32, i32* %b, i64 %idxprom
  store i32 11, i32* %arrayidx, align 4
  br label %if.end

if.else:
  %arrayidx2 = getelementptr inbounds i32, i32* %b, i64 42
  store i32 13, i32* %arrayidx2, align 4
  br label %if.end

if.end:
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; This test checks if the branch conditions and the branch BBs are vectorized
; and masked properly
; CHECK-GE15: define spir_kernel void @__vecz_v4_test_branch(i32 %a, ptr %b)
; CHECK-LT15: define spir_kernel void @__vecz_v4_test_branch(i32 %a, i32* %b)
; CHECK: %conv = sext i32 %a to i64
; CHECK: %[[A_SPLATINSERT:.+]] = insertelement <4 x i64> {{poison|undef}}, i64 %conv, i32 0
; CHECK: %[[A_SPLAT:.+]] = shufflevector <4 x i64> %[[A_SPLATINSERT]], <4 x i64> {{poison|undef}}, <4 x i32> zeroinitializer
; CHECK: %call = call spir_func i64 @_Z13get_global_idj(i32 0)
; CHECK: %[[GID_SPLATINSERT:.+]] = insertelement <4 x i64> {{poison|undef}}, i64 %call, i32 0
; CHECK: %[[GID_SPLAT:.+]] = shufflevector <4 x i64> %[[GID_SPLATINSERT:.+]], <4 x i64> {{poison|undef}}, <4 x i32> zeroinitializer
; CHECK: %[[GID:.+]] = add <4 x i64> %[[GID_SPLAT]], <i64 0, i64 1, i64 2, i64 3>
; CHECK: %[[CMP3:.+]] = icmp eq <4 x i64> %[[A_SPLAT]], %[[GID]]
; CHECK: %[[NOT_CMP4:.+]] = xor <4 x i1> %[[CMP3]], <i1 true, i1 true, i1 true, i1 true>

; CHECK: %[[IDX:.+]] = sext i32 %a to i64
; CHECK-GE15: %[[GEP1:.+]] = getelementptr inbounds i32, ptr %b, i64 %[[IDX]]
; CHECK-LT15: %[[GEP1:.+]] = getelementptr inbounds i32, i32* %b, i64 %[[IDX]]
; CHECK-GE15: call void @__vecz_b_masked_store4_ju3ptrb(i32 11, ptr %[[GEP1]], i1 %{{any_of_mask[0-9]*}})
; CHECK-LT15: call void @__vecz_b_masked_store4_jPjb(i32 11, i32* %[[GEP1]], i1 %{{any_of_mask[0-9]*}})

; CHECK-GE15: %[[GEP2:.+]] = getelementptr inbounds i32, ptr %b, i64 42
; CHECK-LT15: %[[GEP2:.+]] = getelementptr inbounds i32, i32* %b, i64 42
; CHECK-GE15: call void @__vecz_b_masked_store4_ju3ptrb(i32 13, ptr %[[GEP2]], i1 %{{any_of_mask[0-9]*}})
; CHECK-LT15: call void @__vecz_b_masked_store4_jPjb(i32 13, i32* %[[GEP2]], i1 %{{any_of_mask[0-9]*}})

; CHECK: ret void
