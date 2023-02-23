; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc %s --passes='print<generic-metadata>' -S 2>&1 | %filecheck %s

; CHECK: Cached generic metadata analysis:
; CHECK: Kernel Name: add
; CHECK: Local Memory: 0

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
