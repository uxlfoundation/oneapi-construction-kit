; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -vecz-simd-width=4 -S < %s | %filecheck %t 

target triple = "spir64-unknown-unknown"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

declare spir_func i32 @_Z16get_sub_group_idv()
declare spir_func i32 @_Z18get_sub_group_sizev()
declare spir_func i32 @_Z22get_sub_group_local_idv()
declare spir_func i32 @_Z19sub_group_broadcastij(i32, i32)
declare spir_func i64 @_Z13get_global_idj(i32)
declare spir_func i32 @_Z13sub_group_anyi(i32)

define spir_kernel void @get_sub_group_size(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
  %call.i = tail call spir_func i32 @_Z16get_sub_group_idv()
  %conv = zext i32 %call.i to i64
  %call2 = tail call spir_func i32 @_Z18get_sub_group_sizev()
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %conv
  store i32 %call2, i32 addrspace(1)* %arrayidx, align 4
  ret void
; CHECK-LABEL: define spir_kernel void @__vecz_v4_get_sub_group_size(
; CHECK-GE15: store i32 4, ptr addrspace(1) {{.*}}
; CHECK-LT15: store i32 4, i32 addrspace(1)* {{.*}}
}

define spir_kernel void @get_sub_group_local_id(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
  %call = tail call spir_func i32 @_Z22get_sub_group_local_idv()
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out, i32 %call
  store i32 %call, i32 addrspace(1)* %arrayidx, align 4
  ret void
; CHECK-LABEL: define spir_kernel void @__vecz_v4_get_sub_group_local_id(
; CHECK-GE15: store <4 x i32> <i32 0, i32 1, i32 2, i32 3>, ptr addrspace(1) %out
; CHECK-LT15: [[ADDR:%.*]] = bitcast i32 addrspace(1)* %out to <4 x i32> addrspace(1)*
; CHECK-LT15: store <4 x i32> <i32 0, i32 1, i32 2, i32 3>, <4 x i32> addrspace(1)* [[ADDR]]
}

define spir_kernel void @sub_group_broadcast(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
  %call = tail call spir_func i32 @_Z22get_sub_group_local_idv()
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i32 %call
  %v = load i32, i32 addrspace(1)* %arrayidx, align 4
  %broadcast = call spir_func i32 @_Z19sub_group_broadcastij(i32 %v, i32 0)
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %out, i32 %call
  store i32 %broadcast, i32 addrspace(1)* %arrayidx2, align 4
  ret void
; CHECK-LABEL: define spir_kernel void @__vecz_v4_sub_group_broadcast(
; CHECK-GE15: [[LD:%.*]] = load <4 x i32>, ptr addrspace(1) {{%.*}}, align 4
; CHECK-LT15: [[LD:%.*]] = load <4 x i32>, <4 x i32> addrspace(1)* {{%.*}}, align 4
; CHECK: [[SPLAT:%.*]] = shufflevector <4 x i32> [[LD]], <4 x i32> {{(undef|poison)}}, <4 x i32> zeroinitializer
; CHECK-GE15: store <4 x i32> [[SPLAT]], ptr addrspace(1)
; CHECK-LT15: store <4 x i32> [[SPLAT]], <4 x i32> addrspace(1)*
}

; This used to crash as packetizing get_sub_group_local_id produces a Constant, which we weren't expecting.
define spir_kernel void @regression_sub_group_local_id(i32 addrspace(1)* %in, <4 x i32> addrspace(1)* %xy, i32 addrspace(1)* %out) {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %call1 = tail call spir_func i32 @_Z22get_sub_group_local_idv()
  %0 = shl i64 %call, 32
  %idxprom = ashr exact i64 %0, 32
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %xy, i64 %idxprom
  %1 = load <4 x i32>, <4 x i32> addrspace(1)* %arrayidx, align 16
  %2 = insertelement <4 x i32> %1, i32 %call1, i64 0
  %3 = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %arrayidx, i64 0, i64 0
  store i32 %call1, i32 addrspace(1)* %3, align 16
  %call2 = tail call spir_func i32 @_Z16get_sub_group_idv()
  %4 = insertelement <4 x i32> %2, i32 %call2, i64 1
  store <4 x i32> %4, <4 x i32> addrspace(1)* %arrayidx, align 16
  %arrayidx6 = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 %idxprom
  %5 = load i32, i32 addrspace(1)* %arrayidx6, align 4
  %call7 = tail call spir_func i32 @_Z13sub_group_anyi(i32 %5)
  %arrayidx9 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %idxprom
  store i32 %call7, i32 addrspace(1)* %arrayidx9, align 4
  ret void
}

!opencl.ocl.version = !{!0}

!0 = !{i32 3, i32 0}
