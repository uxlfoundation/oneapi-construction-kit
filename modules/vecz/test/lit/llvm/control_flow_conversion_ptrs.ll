; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -vecz-passes=cfg-convert,define-builtins -vecz-simd-width=4 -S < %s | %filecheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @test_varying_if_ptr(i32 %a, i32** %b, i32* %on_true, i32* %on_false) {
entry:
  %conv = sext i32 %a to i64
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %cmp = icmp eq i64 %conv, %call
  br i1 %cmp, label %if.then, label %if.else

if.then:
  %idxprom = sext i32 %a to i64
  %arrayidx = getelementptr inbounds i32*, i32** %b, i64 %idxprom
  store i32* %on_true, i32** %arrayidx, align 4
  br label %if.end

if.else:
  %arrayidx2 = getelementptr inbounds i32*, i32** %b, i64 42
  store i32* %on_false, i32** %arrayidx2, align 4
  br label %if.end

if.end:
  ret void
; CHECK:     define void @__vecz_b_masked_store4_u3ptru3ptrb(ptr [[A:%.*]], ptr [[B:%.*]], i1 [[MASK:%.*]]) {
; CHECK:       br i1 [[MASK]], label %[[IF:.*]], label %[[EXIT:.*]]
; CHECK:     [[IF]]:
; CHECK-NEXT:  store ptr [[A]], ptr [[B]], align 4
; CHECK-NEXT:  br label %[[EXIT]]
; CHECK:     [[EXIT]]:
; CHECK-NEXT:  ret void
}

define spir_kernel void @test_varying_if_ptrptr(i32 %a, i32*** %b, i32** %on_true, i32** %on_false) {
entry:
  %conv = sext i32 %a to i64
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %cmp = icmp eq i64 %conv, %call
  br i1 %cmp, label %if.then, label %if.else

if.then:
  %idxprom = sext i32 %a to i64
  %arrayidx = getelementptr inbounds i32**, i32*** %b, i64 %idxprom
  store i32** %on_true, i32*** %arrayidx, align 4
  br label %if.end

if.else:
  %arrayidx2 = getelementptr inbounds i32**, i32*** %b, i64 42
  store i32** %on_false, i32*** %arrayidx2, align 4
  br label %if.end

if.end:
  ret void
}
