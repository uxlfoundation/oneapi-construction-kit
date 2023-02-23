; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc %s --passes add-vectorize-metadata,verify -S | %filecheck %s

target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n64-S128"
target triple = "riscv64-unknown-unknown-elf"

; CHECK: @notes_global = constant [{{[0-9]+}} x i8] c"{{.*}}", section "notes", align 1

; Function Attrs: nounwind
define void @add() #5 !codeplay_ca_kernel !0 !codeplay_ca_wrapper !2 {
  ret void
}

!0 = !{i32 1}
; Fields for vectorization data are width, isScalable, SimdDimIdx, isVP
; Main vectorization of 1,S
!1 = !{i32 1, i32 1, i32 0, i32 0}
!2 = !{!1, !3}
; Tail is scalar
!3 = !{i32 1, i32 0, i32 0, i32 0}
