; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k mask -vecz-simd-width=16 -S -vecz-choices=TargetIndependentPacketization < %s | %filecheck %t

; ModuleID = 'kernel.opencl'
source_filename = "kernel.opencl"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @mask(i8 addrspace(1)* %out, i8 addrspace(1)* %in, i8 addrspace(1)* %doit) #0 {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0) #2
  %call.tr = trunc i64 %call to i32
  %conv = shl i32 %call.tr, 1
  %idx.ext = sext i32 %conv to i64
  %doit.ptr = getelementptr inbounds i8, i8 addrspace(1)* %doit, i64 %idx.ext
  %ldbool = load i8, i8 addrspace(1)* %doit.ptr, align 1
  %skip = icmp slt i8 %ldbool, 0
  br i1 %skip, label %if.end, label %yes

yes:                                              ; preds = %entry
  %add.ptr = getelementptr inbounds i8, i8 addrspace(1)* %in, i64 %idx.ext
  %0 = load i8, i8 addrspace(1)* %add.ptr, align 1
  %arrayidx1 = getelementptr inbounds i8, i8 addrspace(1)* %add.ptr, i64 1
  %1 = load i8, i8 addrspace(1)* %arrayidx1, align 1
  %add.ptr3 = getelementptr inbounds i8, i8 addrspace(1)* %out, i64 %idx.ext
  %conv4 = sext i8 %0 to i32
  %conv5 = sext i8 %1 to i32
  %add = add nsw i32 %conv5, %conv4
  %cmp = icmp slt i32 %add, 0
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %yes
  %arrayidx7 = getelementptr inbounds i8, i8 addrspace(1)* %add.ptr3, i64 1
  store i8 %0, i8 addrspace(1)* %arrayidx7, align 1
  br label %if.end

if.else:                                          ; preds = %yes
  store i8 %1, i8 addrspace(1)* %add.ptr3, align 1
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then, %entry
  ret void
}

; Function Attrs: convergent nounwind readonly
declare spir_func i64 @_Z13get_global_idj(i32) #1

attributes #0 = { convergent nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="0" "stackrealign" "uniform-work-group-size"="true" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { convergent nounwind readonly "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="0" "stackrealign" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { convergent nobuiltin nounwind readonly }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}
!opencl.kernels = !{!3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 2}
!2 = !{!"clang version 8.0.0 (https://github.com/llvm-mirror/clang.git bfbe338a893dde6ba65b2bed6ffea1652a592819) (https://github.com/llvm-mirror/llvm.git a99d6d2122ca2f208e1c4bcaf02ff5930f244f34)"}
!3 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*)* @mask, !4, !5, !6, !7, !8, !9}
!4 = !{!"kernel_arg_addr_space", i32 1, i32 1}
!5 = !{!"kernel_arg_access_qual", !"none", !"none"}
!6 = !{!"kernel_arg_type", !"char*", !"char*"}
!7 = !{!"kernel_arg_base_type", !"char*", !"char*"}
!8 = !{!"kernel_arg_type_qual", !"", !""}
!9 = !{!"kernel_arg_name", !"out", !"in"}

; This test makes sure we combine a group of masked interleaved stores
; into a single masked interleaved store using interleave operations.
; We expect the interleaved stores to come out unaltered.

; CHECK: entry:
; CHECK: yes:

; The masks get interleaved:
; CHECK: %interleave{{.*}} = shufflevector <16 x i1>
; CHECK: %interleave{{.*}} = shufflevector <16 x i1>

; The loads are masked loads:
; CHECK-GE15: call <16 x i8> @llvm.masked.load.v16i8.p1(ptr
; CHECK-LT15: call <16 x i8> @llvm.masked.load.v16i8.p1v16i8(<16 x i8>
; CHECK-GE15: call <16 x i8> @llvm.masked.load.v16i8.p1(ptr
; CHECK-LT15: call <16 x i8> @llvm.masked.load.v16i8.p1v16i8(<16 x i8>

; The loaded data gets deinterleaved:
; CHECK: %deinterleave{{.*}} = shufflevector <16 x i8>
; CHECK: %deinterleave{{.*}} = shufflevector <16 x i8>

; The data to store gets interleaved:
; CHECK: %interleave{{.*}} = shufflevector <16 x i8>
; CHECK: %interleave{{.*}} = shufflevector <16 x i8>

; The masks get interleaved:
; CHECK: %interleave{{.*}} = shufflevector <16 x i1>
; CHECK: %interleave{{.*}} = shufflevector <16 x i1>

; The stores are masked stores:
; CHECK-GE15: call void @llvm.masked.store.v16i8.p1(<16 x i8>
; CHECK-LT15: call void @llvm.masked.store.v16i8.p1v16i8(<16 x i8>
; CHECK-GE15: call void @llvm.masked.store.v16i8.p1(<16 x i8>
; CHECK-LT15: call void @llvm.masked.store.v16i8.p1v16i8(<16 x i8>

; Definitely no unmasked stores:
; CHECK-NOT: store <16 x i8>
; CHECK ret void
