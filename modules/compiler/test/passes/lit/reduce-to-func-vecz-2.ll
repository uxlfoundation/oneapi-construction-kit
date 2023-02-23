; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes "reduce-to-func<names=A2B_KeepA:A2B_KeepB_Dep:B2A_KeepA_Dep:B2A_KeepB>,verify" -S %s -opaque-pointers \
; RUN:   | %filecheck %s

; This test checks a series of one-way vectorization links, ensuring that we
; don't need two-directional ones to maintain depenent kernels during
; the reduce-to-func pass.

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: declare spir_kernel void @A2B_KeepA_Dep()
declare spir_kernel void @A2B_KeepA_Dep()

; CHECK: define spir_kernel void @A2B_KeepA()
define spir_kernel void @A2B_KeepA() !codeplay_ca_vecz.base !0 {
  ret void
}

; CHECK: declare spir_kernel void @A2B_KeepB_Dep()
declare spir_kernel void @A2B_KeepB_Dep()

; CHECK: define spir_kernel void @A2B_KeepB()
define spir_kernel void @A2B_KeepB() !codeplay_ca_vecz.base !1 {
  ret void
}

; CHECK: declare spir_kernel void @B2A_KeepA_Dep()
declare spir_kernel void @B2A_KeepA_Dep()

; CHECK: define spir_kernel void @B2A_KeepA()
define spir_kernel void @B2A_KeepA() !codeplay_ca_vecz.derived !2 {
  ret void
}

; CHECK: declare spir_kernel void @B2A_KeepB_Dep()
declare spir_kernel void @B2A_KeepB_Dep()

; CHECK: define spir_kernel void @B2A_KeepB()
define spir_kernel void @B2A_KeepB() !codeplay_ca_vecz.derived !3 {
  ret void
}

!0 = !{!4, ptr @A2B_KeepA_Dep}
!1 = !{!4, ptr @A2B_KeepB_Dep}
!2 = !{!4, ptr @B2A_KeepA_Dep}
!3 = !{!4, ptr @B2A_KeepB_Dep}

!4 = !{i32 4, i32 0, i32 0, i32 0}
