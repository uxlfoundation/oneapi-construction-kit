; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test -w 4 -S < %s | %filecheck %t

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir-unknown-unknown"

declare spir_func i32 @get_local_id(i32);
declare spir_func i32 @get_local_size(i32);

define spir_kernel void @test(i32 addrspace(1)* %in) {
entry:
  %id = call i32 @get_local_id(i32 0)
  %size = call i32 @get_local_size(i32 0)
  br label %loop

loop:
  %index = phi i32 [0, %entry], [%inc, %nested_merge]
  br label %koop

koop:
  %kndex = phi i32 [%index, %loop], [%knc, %koop]
  %load = load i32, i32 addrspace(1)* %in
  %slot = getelementptr inbounds i32, i32 addrspace(1)* %in, i32 %index
  store i32 %load, i32 addrspace(1)* %slot
  %knc = add i32 %kndex, 1
  %kmp = icmp ne i32 %knc, %id
  br i1 %kmp, label %koop, label %nested_merge

nested_merge:
  %old = atomicrmw add i32 addrspace(1)* %in, i32 42 acq_rel
  %inc = add i32 %index, 1
  %cmp = icmp ne i32 %inc, %size
  br i1 %cmp, label %loop, label %merge

merge:
  ret void
}

; CHECK: define spir_kernel void @__vecz_v4_test
; CHECK: koop:
; CHECK: %[[BITCAST:[0-9]+]] = bitcast <4 x i1> %koop.entry_mask{{[0-9]*}} to i4
; CHECK: %[[MASK:[^ ]+]] = icmp ne i4 %[[BITCAST]], 0
; CHECK-GE15: %[[LOAD:.+]] = call i32 @__vecz_b_masked_load4_ju3ptrU3AS1b(ptr addrspace(1) %in, i1 %[[MASK]])
; CHECK-GE15: call void @__vecz_b_masked_store4_ju3ptrU3AS1b(i32 %[[LOAD]], ptr addrspace(1) %{{.+}}, i1 %[[MASK]])
; CHECK-LT15: %[[LOAD:.+]] = call i32 @__vecz_b_masked_load4_jPU3AS1jb(i32 addrspace(1)* %in, i1 %[[MASK]])
; CHECK-LT15: call void @__vecz_b_masked_store4_jPU3AS1jb(i32 %[[LOAD]], i32 addrspace(1)* %{{.+}}, i1 %[[MASK]])
; CHECK: nested_merge:
; CHECK-GE15: atomicrmw add ptr addrspace(1) %in, i32 42 acq_rel
; CHECK-GE15: atomicrmw add ptr addrspace(1) %in, i32 42 acq_rel
; CHECK-GE15: atomicrmw add ptr addrspace(1) %in, i32 42 acq_rel
; CHECK-GE15: atomicrmw add ptr addrspace(1) %in, i32 42 acq_rel
; CHECK-LT15: atomicrmw add i32 addrspace(1)* %in, i32 42 acq_rel
; CHECK-LT15: atomicrmw add i32 addrspace(1)* %in, i32 42 acq_rel
; CHECK-LT15: atomicrmw add i32 addrspace(1)* %in, i32 42 acq_rel
; CHECK-LT15: atomicrmw add i32 addrspace(1)* %in, i32 42 acq_rel
