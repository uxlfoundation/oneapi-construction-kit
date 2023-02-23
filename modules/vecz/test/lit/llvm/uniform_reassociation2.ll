; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k uniform_reassociation -vecz-simd-width=4 -S < %s | %filecheck %t

; ModuleID = 'Unknown buffer'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @uniform_reassociation(i32 addrspace(1)* noalias %a, i32 addrspace(1)* noalias %b, i32 addrspace(1)* noalias %d) #0 {
entry:
  %x = call spir_func i64 @_Z13get_global_idj(i32 0) #2
  %y = call spir_func i64 @_Z13get_global_idj(i32 1) #2
  %a_gep = getelementptr inbounds i32, i32 addrspace(1)* %a, i64 %x
  %b_gep = getelementptr inbounds i32, i32 addrspace(1)* %b, i64 %x
  %c_gep = getelementptr inbounds i32, i32 addrspace(1)* %a, i64 %y
  %varying1 = load i32, i32 addrspace(1)* %a_gep
  %varying2 = load i32, i32 addrspace(1)* %b_gep
  %uniform = load i32, i32 addrspace(1)* %c_gep
  %vu = add i32 %varying1, %uniform
  %vvu = add i32 %vu, %varying2
  %d_gep = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %x
  store i32 %vvu, i32 addrspace(1)* %d_gep
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; This test checks that a sum of a varying value with two uniform values
; gets re-associated from (Varying + Uniform) + Varying
; to (Varying + Varying) + Uniform
; CHECK: define spir_kernel void @__vecz_v4_uniform_reassociation

; CHECK: %[[VARYING1:.+]] = load <4 x i32>
; CHECK: %[[VARYING2:.+]] = load <4 x i32>

; The splat of the uniform value
; CHECK: %uniform = load
; CHECK: %[[SPLATINS:.+]] = insertelement <4 x i32> {{undef|poison}}, i32 %uniform, {{(i32|i64)}} 0
; CHECK: %[[SPLAT:.+]] = shufflevector <4 x i32> %[[SPLATINS]], <4 x i32> {{undef|poison}}, <4 x i32> zeroinitializer

; Ensure the two varyings are added together directly
; CHECK: %[[REASSOC:.+]] = add <4 x i32> %[[VARYING1]], %[[VARYING2]]
; CHECK: %[[VVU:.+]] = add <4 x i32> %{{.*}}, %[[SPLAT]]
; CHECK-GE15: store <4 x i32> %[[VVU]], ptr addrspace(1) %{{.+}}
; CHECK-LT15: store <4 x i32> %[[VVU]], <4 x i32> addrspace(1)* %{{.+}}
