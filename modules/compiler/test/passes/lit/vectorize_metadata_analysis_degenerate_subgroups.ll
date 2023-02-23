; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc %s --passes='print<vectorize-metadata>' -S 2>&1 | %filecheck %s

; CHECK: Cached vectorize metadata analysis:
; CHECK-NEXT: Kernel Name: foo
; CHECK-NEXT: Source Name: foo
; CHECK-NEXT: Local Memory: 0
; CHECK-NEXT: Sub-group Size: 0
; CHECK-NEXT: Min Work Width: 1
; CHECK-NEXT: Preferred Work Width: vscale x 1

define void @foo() #0 !codeplay_ca_kernel !0 !codeplay_ca_wrapper !2 {
  ret void
}

attributes #0 = { "mux-degenerate-subgroups" }

!0 = !{i32 1}
; Fields for vectorization data are width, isScalable, SimdDimIdx, isVP
; Main vectorization of 1,S
!1 = !{i32 1, i32 1, i32 0, i32 0}
!2 = !{!1, !3}
; Tail is scalar
!3 = !{i32 1, i32 0, i32 0, i32 0}
