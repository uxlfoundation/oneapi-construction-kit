; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test -w 4 -S < %s | %filecheck %t

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir-unknown-unknown"

declare spir_func i32 @get_local_id(i32);
declare spir_func i32 @get_global_id(i32);

define spir_kernel void @test(i32 addrspace(1)* %in) {
entry:
  %lid = call i32 @get_local_id(i32 0)
  %and = and i32 %lid, 1
  %cmp = icmp eq i32 %and, 0
  br i1 %cmp, label %if, label %merge

if:
  %lid1 = call i32 @get_local_id(i32 1)
  %cmp1 = icmp eq i32 %lid1, 0
  br i1 %cmp1, label %deeper_if, label %deeper_merge

deeper_if:
  br label %deeper_merge

deeper_merge:
  %load = load i32, i32 addrspace(1)* %in
  %gid = call i32 @get_global_id(i32 0)
  %slot = getelementptr inbounds i32, i32 addrspace(1)* %in, i32 %gid
  store i32 %load, i32 addrspace(1)* %slot
  br label %merge

merge:
  ret void
}

; CHECK: define spir_kernel void @__vecz_v4_test
; CHECK-GE15: %[[LOAD:.+]] = load i32, ptr addrspace(1) %in
; CHECK-LT15: %[[LOAD:.+]] = load i32, i32 addrspace(1)* %in
; CHECK: %[[SPLAT_IN:.+]] = insertelement <4 x i32> {{poison|undef}}, i32 %[[LOAD]], {{(i32|i64)}} 0
; CHECK: %[[SPLAT:.+]] = shufflevector <4 x i32> %[[SPLAT_IN]], <4 x i32> {{poison|undef}}, <4 x i32> zeroinitializer
; CHECK-GE15: call void @__vecz_b_masked_store4_Dv4_ju3ptrU3AS1Dv4_b(<4 x i32> %[[SPLAT]], ptr addrspace(1){{( nonnull)? %.*}}, <4 x i1> %{{.+}})
; CHECK-LT15: call void @__vecz_b_masked_store4_Dv4_jPU3AS1Dv4_jDv4_b(<4 x i32> %[[SPLAT]], <4 x i32> addrspace(1)*{{( nonnull)? %.*}}, <4 x i1> %{{.+}})
