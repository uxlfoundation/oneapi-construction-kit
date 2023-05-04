; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes replace-module-scope-vars  -S %s | %filecheck %s

; It checks that a comparison using a global doesn't crash the
; ReplaceLocalModuleScopeVariablesPass

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

@testKernel.lushort = internal addrspace(3) global i16 undef, align 2

declare i64 @__mux_get_global_id(i32)

; Function Attrs: norecurse nounwind
; CHECK: void @testKernel(ptr addrspace(1) nocapture writeonly align 4 %results, ptr [[STRUCT:%.*]])
define spir_kernel void @testKernel(ptr addrspace(1) nocapture writeonly align 4 %results) {
entry:
; CHECK: [[PTR0:%.*]] = getelementptr inbounds %localVarTypes, ptr [[STRUCT]], i32 0, i32 0
; CHECK: [[TMP0:%.*]] = addrspacecast ptr [[PTR0]] to ptr addrspace(3)
; CHECK: %cmp1 = icmp ne ptr addrspace(3) [[TMP0]], null
  %cmp1 = icmp ne ptr addrspace(3) @testKernel.lushort, null
; CHECK: [[PTR1:%.*]] = getelementptr inbounds %localVarTypes, ptr [[STRUCT]], i32 0, i32 0
; CHECK: [[TMP1:%.*]] = addrspacecast ptr [[PTR1]] to ptr addrspace(3)
; CHECK: %cmp2 = icmp ne ptr addrspace(3) [[TMP1]], [[TMP1]]
  %cmp2 = icmp ne ptr addrspace(3) @testKernel.lushort, @testKernel.lushort
  ret void
}
