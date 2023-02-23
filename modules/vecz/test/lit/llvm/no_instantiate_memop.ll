; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k priv -vecz-simd-width=4 -S < %s | %filecheck %t

; ModuleID = 'kernel.opencl'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

; Function Attrs: nounwind
define spir_kernel void @priv(i32 addrspace(3)* %a) #0 {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0) #2
  %conv = trunc i64 %call to i32
  br label %for.cond

for.cond:                                         ; preds = %for.body, %entry
  %storemerge = phi i32 [ 0, %entry ], [ %inc, %for.body ]
  %cmp = icmp ult i32 %storemerge, %conv
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %idxprom = zext i32 %storemerge to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(3)* %a, i64 %idxprom
  store i32 %conv, i32 addrspace(3)* %arrayidx, align 4
  %inc = add i32 %storemerge, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32) #1

attributes #0 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="0" "stackrealign" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="0" "stackrealign" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nobuiltin }

!opencl.kernels = !{!0}
!llvm.ident = !{!6}

!0 = !{void (i32 addrspace(3)*)* @priv, !1, !2, !3, !4, !5}
!1 = !{!"kernel_arg_addr_space", i32 3}
!2 = !{!"kernel_arg_access_qual", !"none"}
!3 = !{!"kernel_arg_type", !"int*"}
!4 = !{!"kernel_arg_base_type", !"int*"}
!5 = !{!"kernel_arg_type_qual", !""}
!6 = !{!"clang version 3.8.0 "}


; Test if the masked store is defined correctly
; CHECK-GE15: call void @__vecz_b_masked_scatter_store4_Dv4_jDv4_u3ptrU3AS3Dv4_b
; CHECK-LT15: call void @__vecz_b_masked_scatter_store4_Dv4_jDv4_PU3AS3jDv4_b
; CHECK: ret void
