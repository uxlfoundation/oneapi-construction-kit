; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(i64 %a, i64 %b, i64* %c) {
entry:
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  %cond = icmp eq i64 %a, %gid
  %c0 = getelementptr i64, i64* %c, i64 %gid
  store i64 %b, i64* %c0, align 4
  %c1 = getelementptr i64, i64* %c, i64 0
  store i64 0, i64* %c1, align 4
  %c2 = select i1 %cond, i64* %c0, i64* %c1
  %c3 = getelementptr i64, i64* %c2, i64 %gid
  %c3.load = load i64, i64* %c3, align 4
  %c4 = getelementptr i64, i64* %c3, i64 %gid
  store i64 %c3.load, i64* %c4, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; Test if the scatter store is defined correctly
; CHECK-GE15: define <4 x i64> @__vecz_b_gather_load4_Dv4_mDv4_u3ptr(<4 x ptr>{{( %0)?}}) {
; CHECK-LT15: define <4 x i64> @__vecz_b_gather_load4_Dv4_mDv4_Pm(<4 x i64*>{{( %0)?}}) {
; CHECK-GE15: call <4 x i64> @llvm.masked.gather.v4i64.v4p0(<4 x ptr> %0, i32{{( immarg)?}} 4, <4 x i1> <i1 true, i1 true, i1 true, i1 true>, <4 x i64> undef)
; CHECK-LT15: call <4 x i64> @llvm.masked.gather.v4i64.v4p0i64(<4 x i64*> %0, i32{{( immarg)?}} 4, <4 x i1> <i1 true, i1 true, i1 true, i1 true>, <4 x i64> undef)
; CHECK: ret <4 x i64>
