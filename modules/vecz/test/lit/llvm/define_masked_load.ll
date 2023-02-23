; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k dont_mask_workitem_builtins -vecz-simd-width=4 -S < %s | %filecheck %t

; ModuleID = 'kernel.opencl'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

; Function Attrs: nounwind
define spir_kernel void @dont_mask_workitem_builtins(i32 addrspace(2)* %in, i32 addrspace(1)* %out) #0 {
entry:
  %call = call spir_func i64 @_Z12get_local_idj(i32 0) #5
  %conv = trunc i64 %call to i32
  %cmp = icmp sgt i32 %conv, 0
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %call2 = call spir_func i64 @_Z13get_global_idj(i32 0) #5
  %conv3 = trunc i64 %call2 to i32
  %idxprom = sext i32 %conv3 to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(2)* %in, i64 %idxprom
  %0 = load i32, i32 addrspace(2)* %arrayidx, align 4
  %idxprom4 = sext i32 %conv3 to i64
  %arrayidx5 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %idxprom4
  store i32 %0, i32 addrspace(1)* %arrayidx5, align 4
  br label %if.end

if.else:                                          ; preds = %entry
  %call8 = call spir_func i64 @_Z14get_local_sizej(i32 0) #5
  %call9 = call spir_func i64 @_Z12get_group_idj(i32 0) #5
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

declare spir_func i64 @_Z12get_local_idj(i32) #1

declare spir_func i64 @_Z13get_global_idj(i32) #1

declare spir_func i64 @_Z14get_local_sizej(i32) #1

declare spir_func i64 @_Z12get_group_idj(i32) #1

attributes #0 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="0" "stackrealign" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="0" "stackrealign" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { noinline }
attributes #3 = { argmemonly nounwind }
attributes #4 = { argmemonly nounwind readonly }
attributes #5 = { nobuiltin nounwind }
attributes #6 = { nounwind }

!opencl.kernels = !{!0}
!llvm.ident = !{!6}

!0 = !{void (i32 addrspace(2)*, i32 addrspace(1)*)* @dont_mask_workitem_builtins, !1, !2, !3, !4, !5}
!1 = !{!"kernel_arg_addr_space", i32 2, i32 1}
!2 = !{!"kernel_arg_access_qual", !"none", !"none"}
!3 = !{!"kernel_arg_type", !"int*", !"int*"}
!4 = !{!"kernel_arg_base_type", !"int*", !"int*"}
!5 = !{!"kernel_arg_type_qual", !"const", !""}
!6 = !{!"clang version 3.8.1 "}



; Test if the masked load is defined correctly
; CHECK-GE15: define <4 x i32> @__vecz_b_masked_load4_Dv4_ju3ptrU3AS2Dv4_b(ptr addrspace(2){{( %0)?}}, <4 x i1>{{( %1)?}})
; CHECK-LT15: define <4 x i32> @__vecz_b_masked_load4_Dv4_jPU3AS2Dv4_jDv4_b(<4 x i32> addrspace(2)*{{( %0)?}}, <4 x i1>{{( %1)?}})
; CHECK: entry:
; CHECK-GE15: %2 = call <4 x i32> @llvm.masked.load.v4i32.p2(ptr addrspace(2) %0, i32{{( immarg)?}} 4, <4 x i1> %1, <4 x i32> undef)
; CHECK-LT15: %2 = call <4 x i32> @llvm.masked.load.v4i32.p2v4i32(<4 x i32> addrspace(2)* %0, i32{{( immarg)?}} 4, <4 x i1> %1, <4 x i32> undef)
; CHECK: ret <4 x i32> %2
