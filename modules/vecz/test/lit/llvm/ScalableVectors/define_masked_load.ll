; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; REQUIRES: llvm-13+
; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k dont_mask_workitem_builtins -vecz-scalable -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @dont_mask_workitem_builtins(i32 addrspace(2)* %in, i32 addrspace(1)* %out) {
entry:
  %call = call spir_func i64 @_Z12get_local_idj(i32 0)
  %conv = trunc i64 %call to i32
  %cmp = icmp sgt i32 %conv, 0
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %call2 = call spir_func i64 @_Z13get_global_idj(i32 0)
  %conv3 = trunc i64 %call2 to i32
  %idxprom = sext i32 %conv3 to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(2)* %in, i64 %idxprom
  %0 = load i32, i32 addrspace(2)* %arrayidx, align 4
  %idxprom4 = sext i32 %conv3 to i64
  %arrayidx5 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %idxprom4
  store i32 %0, i32 addrspace(1)* %arrayidx5, align 4
  br label %if.end

if.else:                                          ; preds = %entry
  %call8 = call spir_func i64 @_Z14get_local_sizej(i32 0)
  %call9 = call spir_func i64 @_Z12get_group_idj(i32 0)
  %mul = mul i64 %call9, %call8
  %add = add i64 %mul, %call
  %sext = shl i64 %add, 32
  %idxprom11 = ashr exact i64 %sext, 32
  %arrayidx12 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %idxprom11
  store i32 42, i32 addrspace(1)* %arrayidx12, align 4
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  ret void
}

declare spir_func void @_Z7barrierj(i32)

declare spir_func i64 @_Z12get_local_idj(i32)

declare spir_func i64 @_Z13get_global_idj(i32)

declare spir_func i64 @_Z14get_local_sizej(i32)

declare spir_func i64 @_Z12get_group_idj(i32)

; Test if the masked load is defined correctly
; CHECK-GE15: define <vscale x 4 x i32> @__vecz_b_masked_load4_u5nxv4ju3ptrU3AS2u5nxv4b(ptr addrspace(2){{( %0)?}}, <vscale x 4 x i1>{{( %1)?}})
; CHECK-LT15: define <vscale x 4 x i32> @__vecz_b_masked_load4_u5nxv4jPU3AS2u5nxv4ju5nxv4b(<vscale x 4 x i32> addrspace(2)*{{( %0)?}}, <vscale x 4 x i1>{{( %1)?}})
; CHECK: entry:
; CHECK-GE15: %2 = call <vscale x 4 x i32> @llvm.masked.load.nxv4i32.p2(ptr addrspace(2) %0, i32{{( immarg)?}} 4, <vscale x 4 x i1> %1, <vscale x 4 x i32> undef)
; CHECK-LT15: %2 = call <vscale x 4 x i32> @llvm.masked.load.nxv4i32.p2nxv4i32(<vscale x 4 x i32> addrspace(2)* %0, i32{{( immarg)?}} 4, <vscale x 4 x i1> %1, <vscale x 4 x i32> undef)
; CHECK: ret <vscale x 4 x i32> %2
