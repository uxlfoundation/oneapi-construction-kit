; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -k test -w 4 -S < %s | %filecheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir-unknown-unknown"

declare spir_func void @barrier(i32);
declare spir_func i32 @get_local_id(i32);
declare spir_func i32 @get_global_id(i32);

define spir_kernel void @test(i32 addrspace(1)* %in) {
entry:
  %load = load i32, i32 addrspace(1)* %in
  %gid = call i32 @get_global_id(i32 0)
  %slot = getelementptr inbounds i32, i32 addrspace(1)* %in, i32 %gid
  store i32 %load, i32 addrspace(1)* %slot

  ret void
}

; CHECK: define spir_kernel void @__vecz_v4_test
; CHECK: entry:
; CHECK: %load = load i32, ptr addrspace(1) %in
