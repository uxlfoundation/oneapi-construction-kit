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
  %b_gep = getelementptr inbounds i32, i32 addrspace(1)* %b, i64 %y
  %c_gep = getelementptr inbounds i32, i32 addrspace(1)* %a, i64 %y
  %varying = load i32, i32 addrspace(1)* %a_gep
  %uniform1 = load i32, i32 addrspace(1)* %b_gep
  %uniform2 = load i32, i32 addrspace(1)* %c_gep
  %vu = add i32 %varying, %uniform1
  %vuu = add i32 %vu, %uniform2
  %d_gep = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %x
  store i32 %vuu, i32 addrspace(1)* %d_gep
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; This test checks that a sum of a varying value with two uniform values
; gets re-associated from (Varying + Uniform) + Uniform
; to Varying + (Uniform + Uniform)
; CHECK: define spir_kernel void @__vecz_v4_uniform_reassociation
; CHECK: load

; Ensure the two uniforms are added together directly
; CHECK: %[[REASSOC:.+]] = add i32 %uniform1, %uniform2

; Ensure there is only one vector splat
; CHECK: %[[SPLATINS:.+]] = insertelement <4 x i32> {{undef|poison}}, i32 %[[REASSOC]], {{(i32|i64)}} 0
; CHECK-NOT: insertelement <4 x i32> {{undef|poison}}, i32 %{{.+}}, {{(i32|i64)}} 0

; CHECK: %[[SPLAT:.+]] = shufflevector <4 x i32> %[[SPLATINS]], <4 x i32> {{undef|poison}}, <4 x i32> zeroinitializer
; CHECK: %[[RESULT:.+]] = add <4 x i32> %{{.*}}, %[[SPLAT]]
; CHECK-GE15: store <4 x i32> %vuu{{.*}}, ptr addrspace(1) %{{.+}}
; CHECK-LT15: store <4 x i32> %vuu{{.*}}, <4 x i32> addrspace(1)* %{{.+}}
