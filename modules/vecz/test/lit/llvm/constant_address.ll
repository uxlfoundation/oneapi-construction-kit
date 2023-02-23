; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test -w 4 -S < %s | %filecheck %t

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(i32 addrspace(1)* %out) #0 {
entry:
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0) #1
  %conv = trunc i64 %gid to i32
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 3
  store i32 %conv, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32) #1

attributes #0 = { nounwind }
attributes #1 = { nounwind readnone }

!opencl.kernels = !{!0}
!opencl.spir.version = !{!7}
!opencl.ocl.version = !{!7}
!opencl.used.extensions = !{!8}
!opencl.used.optional.core.features = !{!8}
!opencl.compiler.options = !{!8}

!0 = !{void (i32 addrspace(1)*)* @test, !1, !2, !3, !4, !5, !6}
!1 = !{!"kernel_arg_addr_space", i32 1}
!2 = !{!"kernel_arg_access_qual", !"none"}
!3 = !{!"kernel_arg_type", !"int*"}
!4 = !{!"kernel_arg_base_type", !"int*"}
!5 = !{!"kernel_arg_type_qual", !""}
!6 = !{!"kernel_arg_name", !"out"}
!7 = !{i32 1, i32 2}
!8 = !{}

; CHECK: define spir_kernel void @__vecz_v4_test
; CHECK-NEXT: entry:
; CHECK-NEXT: %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
; CHECK-NEXT: %conv = trunc i64 %gid to i32
; CHECK-NEXT-GE15: %arrayidx = getelementptr inbounds i32, ptr addrspace(1) %out, i64 3
; CHECK-NEXT-LT15: %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 3
; CHECK-NEXT-GE15: store i32 %conv, ptr addrspace(1) %arrayidx, align 4
; CHECK-NEXT-LT15: store i32 %conv, i32 addrspace(1)* %arrayidx, align 4
