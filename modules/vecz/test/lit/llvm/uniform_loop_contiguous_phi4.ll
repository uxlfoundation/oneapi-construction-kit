; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test -w 4 -S < %s | %filecheck %t

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir-unknown-unknown"

define spir_kernel void @test(i32 addrspace(1)* %in) {
entry:
  %id = call spir_func i64 @_Z13get_global_idj(i64 0) #2
  %init_addr = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 %id
  %load = load i32, i32 addrspace(1)* %init_addr
  br label %loop

loop:
  %count = phi i64 [0, %entry], [%inc, %loop]
  %index = phi i64 [%inc_index, %loop], [%id, %entry]
  %slot = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 %index
  store i32 %load, i32 addrspace(1)* %slot
  %inc_index = add i64 %index, 16
  %inc = add i64 %count, 1
  %cmp = icmp ne i64 %inc, 16
  br i1 %cmp, label %loop, label %merge

merge:
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i64)

; It checks that the stride analysis can tell the store is contiguous through the PHI node.
; Same as uniform_loop_contiguous_phi3.ll except with the PHI node incoming values reversed.

; CHECK: define spir_kernel void @__vecz_v4_test
; CHECK-GE15: %[[LD:.+]] = load <4 x i32>, ptr addrspace(1) %init_addr
; CHECK-LT15: %[[VEC:.+]] = bitcast i32 addrspace(1)* %init_addr to <4 x i32> addrspace(1)*
; CHECK-LT15: %[[LD:.+]] = load <4 x i32>, <4 x i32> addrspace(1)* %[[VEC]]
; CHECK: loop:
; CHECK-GE15: store <4 x i32> %[[LD]], ptr addrspace(1) %slot
; CHECK-LT15: %[[VEC2:.+]] = bitcast i32 addrspace(1)* %slot to <4 x i32> addrspace(1)*
; CHECK-LT15: store <4 x i32> %[[LD]], <4 x i32> addrspace(1)* %[[VEC2]]
