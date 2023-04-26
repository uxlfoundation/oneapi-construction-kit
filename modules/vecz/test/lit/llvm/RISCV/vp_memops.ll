; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; REQUIRES: llvm-13+
; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k store_element -vecz-target-triple="riscv64-unknown-unknown" -vecz-target-features=+f,+d,%vattr -vecz-simd-width=4 -vecz-scalable -vecz-choices=VectorPredication -S < %s | %filecheck %t --check-prefix CHECK-STORE-4
; RUN: %veczc -k store_element -vecz-target-triple="riscv64-unknown-unknown" -vecz-target-features=+f,+d,%vattr -vecz-simd-width=8 -vecz-scalable -vecz-choices=VectorPredication -S < %s | %filecheck %t --check-prefix CHECK-STORE-8
; RUN: %veczc -k store_element -vecz-target-triple="riscv64-unknown-unknown" -vecz-target-features=+f,+d,%vattr -vecz-simd-width=16 -vecz-scalable -vecz-choices=VectorPredication -S < %s | %filecheck %t --check-prefix CHECK-STORE-16
; RUN: %veczc -k load_element -vecz-target-triple="riscv64-unknown-unknown" -vecz-target-features=+f,+d,%vattr -vecz-simd-width=4 -vecz-scalable -vecz-choices=VectorPredication -S < %s | %filecheck %t --check-prefix CHECK-LOAD-4
; RUN: %veczc -k load_element -vecz-target-triple="riscv64-unknown-unknown" -vecz-target-features=+f,+d,%vattr -vecz-simd-width=8 -vecz-scalable -vecz-choices=VectorPredication -S < %s | %filecheck %t --check-prefix CHECK-LOAD-8
; RUN: %veczc -k load_element -vecz-target-triple="riscv64-unknown-unknown" -vecz-target-features=+f,+d,%vattr -vecz-simd-width=16 -vecz-scalable -vecz-choices=VectorPredication -S < %s | %filecheck %t --check-prefix CHECK-LOAD-16

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @store_element(i32 %0, i32 addrspace(1)* %b) {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %cond = icmp ne i64 %call, 0
  br i1 %cond, label %do, label %ret

do:
  %dest = getelementptr inbounds i32, i32 addrspace(1)* %b, i64 %call
  store i32 %0, i32 addrspace(1)* %dest, align 4
  br label %ret

ret:
  ret void
}

; CHECK-STORE-4-GE15:       define void @__vecz_b_masked_store4_vp_u5nxv4ju3ptrU3AS1u5nxv4bj(<vscale x 4 x i32> [[TMP0:%.*]], ptr addrspace(1) [[TMP1:%.*]], <vscale x 4 x i1> [[TMP2:%.*]], i32 [[TMP3:%.*]]) {
; CHECK-STORE-4-LT15:       define void @__vecz_b_masked_store4_vp_u5nxv4jPU3AS1u5nxv4ju5nxv4bj(<vscale x 4 x i32> [[TMP0:%.*]], <vscale x 4 x i32> addrspace(1)* [[TMP1:%.*]], <vscale x 4 x i1> [[TMP2:%.*]], i32 [[TMP3:%.*]]) {
; CHECK-STORE-4-NEXT:  entry:
; CHECK-STORE-4-NEXT-GE15:    call void @llvm.vp.store.nxv4i32.p1(<vscale x 4 x i32> [[TMP0]], ptr addrspace(1) [[TMP1]], <vscale x 4 x i1> [[TMP2]], i32 [[TMP3]])
; CHECK-STORE-4-NEXT-LT15:    call void @llvm.vp.store.nxv4i32.p1nxv4i32(<vscale x 4 x i32> [[TMP0]], <vscale x 4 x i32> addrspace(1)* [[TMP1]], <vscale x 4 x i1> [[TMP2]], i32 [[TMP3]])
; CHECK-STORE-4-NEXT:    ret void

; CHECK-STORE-8-GE15:       define void @__vecz_b_masked_store4_vp_u5nxv8ju3ptrU3AS1u5nxv8bj(<vscale x 8 x i32> [[TMP0:%.*]], ptr addrspace(1) [[TMP1:%.*]], <vscale x 8 x i1> [[TMP2:%.*]], i32 [[TMP3:%.*]]) {
; CHECK-STORE-8-LT15:       define void @__vecz_b_masked_store4_vp_u5nxv8jPU3AS1u5nxv8ju5nxv8bj(<vscale x 8 x i32> [[TMP0:%.*]], <vscale x 8 x i32> addrspace(1)* [[TMP1:%.*]], <vscale x 8 x i1> [[TMP2:%.*]], i32 [[TMP3:%.*]]) {
; CHECK-STORE-8-NEXT:  entry:
; CHECK-STORE-8-NEXT-GE15:    call void @llvm.vp.store.nxv8i32.p1(<vscale x 8 x i32> [[TMP0]], ptr addrspace(1) [[TMP1]], <vscale x 8 x i1> [[TMP2]], i32 [[TMP3]])
; CHECK-STORE-8-NEXT-LT15:    call void @llvm.vp.store.nxv8i32.p1nxv8i32(<vscale x 8 x i32> [[TMP0]], <vscale x 8 x i32> addrspace(1)* [[TMP1]], <vscale x 8 x i1> [[TMP2]], i32 [[TMP3]])
; CHECK-STORE-8-NEXT:    ret void

; CHECK-STORE-16-GE15:       define void @__vecz_b_masked_store4_vp_u6nxv16ju3ptrU3AS1u6nxv16bj(<vscale x 16 x i32> [[TMP0:%.*]], ptr addrspace(1) [[TMP1:%.*]], <vscale x 16 x i1> [[TMP2:%.*]], i32 [[TMP3:%.*]]) {
; CHECK-STORE-16-LT15:       define void @__vecz_b_masked_store4_vp_u6nxv16jPU3AS1u6nxv16ju6nxv16bj(<vscale x 16 x i32> [[TMP0:%.*]], <vscale x 16 x i32> addrspace(1)* [[TMP1:%.*]], <vscale x 16 x i1> [[TMP2:%.*]], i32 [[TMP3:%.*]]) {
; CHECK-STORE-16-NEXT:  entry:
; CHECK-STORE-16-NEXT:    [[TMP5:%.*]] = call <vscale x 16 x i32> @llvm.experimental.stepvector.nxv16i32()
; CHECK-STORE-16-NEXT:    [[SPLATINSERT:%.*]] = insertelement <vscale x 16 x i32> poison, i32 [[TMP3]], {{i32|i64}} 0
; CHECK-STORE-16-NEXT:    [[SPLAT:%.*]] = shufflevector <vscale x 16 x i32> [[SPLATINSERT]], <vscale x 16 x i32> poison, <vscale x 16 x i32> zeroinitializer
; CHECK-STORE-16-NEXT:    [[TMP6:%.*]] = icmp ult <vscale x 16 x i32> [[TMP5]], [[SPLAT]]
; CHECK-STORE-16-NEXT:    [[TMP7:%.*]] = select <vscale x 16 x i1> [[TMP2]], <vscale x 16 x i1> [[TMP6]], <vscale x 16 x i1> zeroinitializer
; CHECK-STORE-16-NEXT-GE15:    call void @llvm.masked.store.nxv16i32.p1(<vscale x 16 x i32> [[TMP0]], ptr addrspace(1) [[TMP1]], i32 4, <vscale x 16 x i1> [[TMP7]])
; CHECK-STORE-16-NEXT-LT15:    call void @llvm.masked.store.nxv16i32.p1nxv16i32(<vscale x 16 x i32> [[TMP0]], <vscale x 16 x i32> addrspace(1)* [[TMP1]], i32 4, <vscale x 16 x i1> [[TMP7]])
; CHECK-STORE-16-NEXT:    ret void

define spir_kernel void @load_element(i32 addrspace(1)* %a, i32 addrspace(1)* %b) {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %cond = icmp ne i64 %call, 0
  br i1 %cond, label %do, label %ret

do:
  %src = getelementptr inbounds i32, i32 addrspace(1)* %a, i64 %call
  %dest = getelementptr inbounds i32, i32 addrspace(1)* %b, i64 %call
  %do.res = load i32, i32 addrspace(1)* %src, align 4
  store i32 %do.res, i32 addrspace(1)* %dest, align 4
  br label %ret

ret:
  ret void
}

; CHECK-LOAD-4-GE15:      define <vscale x 4 x i32> @__vecz_b_masked_load4_vp_u5nxv4ju3ptrU3AS1u5nxv4bj(ptr addrspace(1) [[TMP0:%.*]], <vscale x 4 x i1> [[TMP1:%.*]], i32 [[TMP2:%.*]]) {
; CHECK-LOAD-4-LT15:      define <vscale x 4 x i32> @__vecz_b_masked_load4_vp_u5nxv4jPU3AS1u5nxv4ju5nxv4bj(<vscale x 4 x i32> addrspace(1)* [[TMP0:%.*]], <vscale x 4 x i1> [[TMP1:%.*]], i32 [[TMP2:%.*]]) {
; CHECK-LOAD-4-NEXT: entry:
; CHECK-LOAD-4-NEXT-GE15:   [[TMP4:%.*]] = call <vscale x 4 x i32> @llvm.vp.load.nxv4i32.p1(ptr addrspace(1) [[TMP0]], <vscale x 4 x i1> [[TMP1]], i32 [[TMP2]])
; CHECK-LOAD-4-NEXT-LT15:   [[TMP4:%.*]] = call <vscale x 4 x i32> @llvm.vp.load.nxv4i32.p1nxv4i32(<vscale x 4 x i32> addrspace(1)* [[TMP0]], <vscale x 4 x i1> [[TMP1]], i32 [[TMP2]])
; CHECK-LOAD-4-NEXT:   ret <vscale x 4 x i32> [[TMP4]]

; CHECK-LOAD-8-GE15:      define <vscale x 8 x i32> @__vecz_b_masked_load4_vp_u5nxv8ju3ptrU3AS1u5nxv8bj(ptr addrspace(1) [[TMP0:%.*]], <vscale x 8 x i1> [[TMP1:%.*]], i32 [[TMP2:%.*]]) {
; CHECK-LOAD-8-LT15:      define <vscale x 8 x i32> @__vecz_b_masked_load4_vp_u5nxv8jPU3AS1u5nxv8ju5nxv8bj(<vscale x 8 x i32> addrspace(1)* [[TMP0:%.*]], <vscale x 8 x i1> [[TMP1:%.*]], i32 [[TMP2:%.*]]) {
; CHECK-LOAD-8-NEXT: entry:
; CHECK-LOAD-8-NEXT-GE15:   [[TMP4:%.*]] = call <vscale x 8 x i32> @llvm.vp.load.nxv8i32.p1(ptr addrspace(1) [[TMP0]], <vscale x 8 x i1> [[TMP1]], i32 [[TMP2]])
; CHECK-LOAD-8-NEXT-LT15:   [[TMP4:%.*]] = call <vscale x 8 x i32> @llvm.vp.load.nxv8i32.p1nxv8i32(<vscale x 8 x i32> addrspace(1)* [[TMP0]], <vscale x 8 x i1> [[TMP1]], i32 [[TMP2]])
; CHECK-LOAD-8-NEXT:   ret <vscale x 8 x i32> [[TMP4]]

; CHECK-LOAD-16-GE15:      define <vscale x 16 x i32> @__vecz_b_masked_load4_vp_u6nxv16ju3ptrU3AS1u6nxv16bj(ptr addrspace(1) [[TMP0:%.*]], <vscale x 16 x i1> [[TMP1:%.*]], i32 [[TMP2:%.*]]) {
; CHECK-LOAD-16-LT15:      define <vscale x 16 x i32> @__vecz_b_masked_load4_vp_u6nxv16jPU3AS1u6nxv16ju6nxv16bj(<vscale x 16 x i32> addrspace(1)* [[TMP0:%.*]], <vscale x 16 x i1> [[TMP1:%.*]], i32 [[TMP2:%.*]]) {
; CHECK-LOAD-16-NEXT: entry:
; CHECK-LOAD-16-NEXT: [[TMP4:%.*]] = call <vscale x 16 x i32> @llvm.experimental.stepvector.nxv16i32()
; CHECK-LOAD-16-NEXT: [[TMPSPLATINSERT:%.*]] = insertelement <vscale x 16 x i32> poison, i32 [[TMP2]], {{i32|i64}} 0
; CHECK-LOAD-16-NEXT: [[TMPSPLAT:%.*]] = shufflevector <vscale x 16 x i32> [[TMPSPLATINSERT]], <vscale x 16 x i32> poison, <vscale x 16 x i32> zeroinitializer
; CHECK-LOAD-16-NEXT: [[TMP5:%.*]] = icmp ult <vscale x 16 x i32> [[TMP4]], [[TMPSPLAT]]
; CHECK-LOAD-16-NEXT: [[TMP6:%.*]] = select <vscale x 16 x i1> [[TMP1]], <vscale x 16 x i1> [[TMP5]], <vscale x 16 x i1> zeroinitializer
; CHECK-LOAD-16-NEXT-GE15: [[TMP7:%.*]] = call <vscale x 16 x i32> @llvm.masked.load.nxv16i32.p1(ptr addrspace(1) [[TMP0]], i32 4, <vscale x 16 x i1> [[TMP6]], <vscale x 16 x i32> undef)
; CHECK-LOAD-16-NEXT-LT15: [[TMP7:%.*]] = call <vscale x 16 x i32> @llvm.masked.load.nxv16i32.p1nxv16i32(<vscale x 16 x i32> addrspace(1)* [[TMP0]], i32 4, <vscale x 16 x i1> [[TMP6]], <vscale x 16 x i32> undef)
; CHECK-LOAD-16-NEXT: ret <vscale x 16 x i32> [[TMP7]]

declare spir_func i64 @_Z13get_global_idj(i32)
