; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -k test -w 4 -S < %s | %filecheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir-unknown-unknown"

declare spir_func i32 @get_local_size(i32);

define spir_kernel void @test(i32 addrspace(1)* %in) {
entry:
  %size = call i32 @get_local_size(i32 0)
  br label %loop

loop:
  %index = phi i32 [0, %entry], [%inc, %loop]
  %load = load i32, i32 addrspace(1)* %in
  %slot = getelementptr inbounds i32, i32 addrspace(1)* %in, i32 %index
  store i32 %load, i32 addrspace(1)* %slot
  %inc = add i32 %index, 1
  %cmp = icmp ne i32 %inc, %size
  br i1 %cmp, label %loop, label %merge

merge:
  ret void
}

; CHECK: define spir_kernel void @test(ptr addrspace(1) %in) !codeplay_ca_vecz.base !0
; CHECK: entry:
; CHECK: loop:
; CHECK: define spir_kernel void @__vecz_v4_test(ptr addrspace(1) %in) #0 !codeplay_ca_vecz.derived !2
; CHECK: entry:
; CHECK: loop:
; CHECK: !0 = !{!1, ptr @__vecz_v4_test}
; CHECK: !1 = !{i32 4, i32 0, i32 0, i32 0}
; CHECK: !2 = !{!1, ptr @test}
