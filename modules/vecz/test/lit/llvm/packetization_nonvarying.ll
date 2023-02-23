; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test_nonvarying_loadstore -vecz-passes=packetizer -vecz-simd-width=4 -S < %s | %filecheck %t

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

define spir_kernel void @test_uniform_branch(i32 %a, i32* %b) {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %cmp = icmp eq i32 %a, 42
  br i1 %cmp, label %if.then, label %if.else

if.then:
  %idxprom = sext i32 %a to i64
  %idxadd = add i64 %idxprom, %call
  %arrayidx = getelementptr inbounds i32, i32* %b, i64 %idxadd
  store i32 11, i32* %arrayidx, align 4
  br label %if.end

if.else:
  %arrayidx2 = getelementptr inbounds i32, i32* %b, i64 %call
  store i32 13, i32* %arrayidx2, align 4
  br label %if.end

if.end:
  %ptr = phi i32* [ %arrayidx, %if.then ], [ %arrayidx2, %if.else ]
  %ptrplus = getelementptr inbounds i32, i32* %ptr, i64 %call
  store i32 17, i32* %ptrplus, align 4
  ret void
}

define spir_func void @test_nonvarying_loadstore(i32* %a, i32* %b, i32* %c) {
  %index = call spir_func i64 @_Z13get_global_idj(i32 0)
  %a.i = getelementptr i32, i32* %a, i64 %index
  %b.i = getelementptr i32, i32* %b, i64 %index
  %c.i = getelementptr i32, i32* %c, i64 %index
  %a.load = load i32, i32* %a.i, align 4
  %b.load = load i32, i32* %b.i, align 4
  %add = add i32 %a.load, %b.load
  store i32 %add, i32* %c.i
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; This test checks if a simple kernel is vectorized without any masks
; CHECK-GE15: define spir_func void @__vecz_v4_test_nonvarying_loadstore(ptr %a, ptr %b, ptr %c)
; CHECK-LT15: define spir_func void @__vecz_v4_test_nonvarying_loadstore(i32* %a, i32* %b, i32* %c)
; CHECK: %index = call spir_func i64 @_Z13get_global_idj(i32 0)
; CHECK-GE15: %a.i = getelementptr i32, ptr %a, i64 %index
; CHECK-LT15: %a.i = getelementptr i32, i32* %a, i64 %index
; CHECK-GE15: %b.i = getelementptr i32, ptr %b, i64 %index
; CHECK-LT15: %b.i = getelementptr i32, i32* %b, i64 %index
; CHECK-GE15: %c.i = getelementptr i32, ptr %c, i64 %index
; CHECK-LT15: %c.i = getelementptr i32, i32* %c, i64 %index
; CHECK-GE15: %[[LAV:.+]] = load <4 x i32>, ptr %a.i{{(, align 4)?}}
; CHECK-LT15: %[[AV:.+]] = bitcast i32* %a.i to <4 x i32>*
; CHECK-LT15: %[[LAV:.+]] = load <4 x i32>, <4 x i32>* %[[AV]]{{(, align 4)?}}
; CHECK-GE15: %[[LBV:.+]] = load <4 x i32>, ptr %b.i{{(, align 4)?}}
; CHECK-LT15: %[[BV:.+]] = bitcast i32* %b.i to <4 x i32>*
; CHECK-LT15: %[[LBV:.+]] = load <4 x i32>, <4 x i32>* %[[BV]]{{(, align 4)?}}
; CHECK: %[[ADD1:.+]] = add <4 x i32> %[[LAV]], %[[LBV]]
; CHECK-GE15: store <4 x i32> %[[ADD1]], ptr %c.i{{(, align 4)?}}
; CHECK-LT15: %[[CV:.+]] = bitcast i32* %c.i to <4 x i32>*
; CHECK-LT15: store <4 x i32> %[[ADD1]], <4 x i32>* %[[CV]]{{(, align 4)?}}
; CHECK: ret void
