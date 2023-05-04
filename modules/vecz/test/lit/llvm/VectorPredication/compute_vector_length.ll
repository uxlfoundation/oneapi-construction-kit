; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; REQUIRES: llvm-13+
; RUN: %veczc -k get_sub_group_size -vecz-simd-width=2 -vecz-choices=VectorPredication -S < %s | %filecheck %s --check-prefix CHECK-F2
; RUN: %veczc -k get_sub_group_size -vecz-scalable -vecz-simd-width=4 -vecz-choices=VectorPredication -S < %s | %filecheck %s --check-prefix CHECK-S4

target triple = "spir64-unknown-unknown"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

declare spir_func i32 @_Z16get_sub_group_idv()
declare spir_func i32 @_Z18get_sub_group_sizev()

define spir_kernel void @get_sub_group_size(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
  %call.i = tail call spir_func i32 @_Z16get_sub_group_idv()
  %conv = zext i32 %call.i to i64
  %call2 = tail call spir_func i32 @_Z18get_sub_group_sizev()
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %conv
  store i32 %call2, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Makes sure the vector length is properly computed and substituted for get_sub_group_size()

; CHECK-F2-LABEL: define spir_kernel void @__vecz_v2_vp_get_sub_group_size(
; CHECK-F2: [[ID:%.*]] = call i64 @__mux_get_local_id(i32 0)
; CHECK-F2: [[SZ:%.*]] = call i64 @__mux_get_local_size(i32 0)
; CHECK-F2: [[WL:%.*]] = sub {{.*}} i64 [[SZ]], [[ID]]
; CHECK-F2: [[VL0:%.*]] = call i64 @llvm.umin.i64(i64 [[WL]], i64 2)
; CHECK-F2: [[VL1:%.*]] = trunc i64 [[VL0]] to i32
; CHECK-F2: store i32 [[VL1]], ptr addrspace(1) {{.*}}

; CHECK-S4-LABEL: define spir_kernel void @__vecz_nxv4_vp_get_sub_group_size(
; CHECK-S4: [[ID:%.*]] = call i64 @__mux_get_local_id(i32 0)
; CHECK-S4: [[SZ:%.*]] = call i64 @__mux_get_local_size(i32 0)
; CHECK-S4: [[WL:%.*]] = sub {{.*}} i64 [[SZ]], [[ID]]
; CHECK-S4: [[VF0:%.*]] = call i64 @llvm.vscale.i64()
; CHECK-S4: [[VF1:%.*]] = shl i64 [[VF0]], 2
; CHECK-S4: [[VL0:%.*]] = call i64 @llvm.umin.i64(i64 [[WL]], i64 [[VF1]])
; CHECK-S4: [[VL1:%.*]] = trunc i64 [[VL0]] to i32
; CHECK-S4: store i32 [[VL1]], ptr addrspace(1) {{.*}}
