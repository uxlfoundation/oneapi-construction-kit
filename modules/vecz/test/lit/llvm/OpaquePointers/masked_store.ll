; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -vecz-passes=cfg-convert,define-builtins -vecz-simd-width=4 %flag -S < %s | %filecheck %s

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @test_varying_if(i32 %a, ptr %b, float %on_true, float %on_false) {
entry:
  %conv = sext i32 %a to i64
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %cmp = icmp eq i64 %conv, %call
  br i1 %cmp, label %if.then, label %if.else

if.then:
  %idxprom = sext i32 %a to i64
  %arrayidx = getelementptr inbounds ptr, ptr %b, i64 %idxprom
  store float %on_true, ptr %arrayidx, align 4
  br label %if.end

if.else:
  %arrayidx2 = getelementptr inbounds ptr, ptr %b, i64 42
  store float %on_false, ptr %arrayidx2, align 4
  br label %if.end

if.end:
  ret void
}

define spir_kernel void @test_varying_if_as3(i32 %a, ptr addrspace(3) %b, float %on_true, float %on_false) {
entry:
  %conv = sext i32 %a to i64
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %cmp = icmp eq i64 %conv, %call
  br i1 %cmp, label %if.then, label %if.else

if.then:
  %idxprom = sext i32 %a to i64
  %arrayidx = getelementptr inbounds ptr, ptr addrspace(3) %b, i64 %idxprom
  store float %on_true, ptr addrspace(3) %arrayidx, align 4
  br label %if.end

if.else:
  %arrayidx2 = getelementptr inbounds ptr, ptr addrspace(3) %b, i64 42
  store float %on_false, ptr addrspace(3) %arrayidx2, align 4
  br label %if.end

if.end:
  ret void
}

; CHECK:     define void @__vecz_b_masked_store4_fu3ptrb(float [[A:%.*]], ptr [[B:%.*]], i1 [[MASK:%.*]]) {
; CHECK:       br i1 [[MASK]], label %[[IF:.*]], label %[[EXIT:.*]]
; CHECK:     [[IF]]:
; CHECK-NEXT:  store float [[A]], ptr [[B]], align 4
; CHECK-NEXT:  br label %[[EXIT]]
; CHECK:     [[EXIT]]:
; CHECK-NEXT:  ret void

; CHECK:     define void @__vecz_b_masked_store4_fu3ptrU3AS3b(float [[A:%.*]], ptr addrspace(3) [[B:%.*]], i1 [[MASK:%.*]]) {
; CHECK:       br i1 [[MASK]], label %[[IF:.*]], label %[[EXIT:.*]]
; CHECK:     [[IF]]:
; CHECK-NEXT:  store float [[A]], ptr addrspace(3) [[B]], align 4
; CHECK-NEXT:  br label %[[EXIT]]
; CHECK:     [[EXIT]]:
; CHECK-NEXT:  ret void
