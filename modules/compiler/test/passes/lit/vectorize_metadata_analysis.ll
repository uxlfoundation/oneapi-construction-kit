; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc %s --passes='print<vectorize-metadata>' -S 2>&1 | %filecheck %s

; CHECK: Cached vectorize metadata analysis:
; CHECK:      Kernel Name: kernel1
; CHECK-NEXT: Source Name: kernel1
; CHECK-NEXT: Local Memory: 0
; CHECK-NEXT: Sub-group Size: vscale x 1
; CHECK-NEXT: Min Work Width: 1
; CHECK-NEXT: Preferred Work Width: vscale x 1

; CHECK:      Kernel Name: kernel2
; CHECK-NEXT: Source Name: test
; CHECK-NEXT: Local Memory: 0
; CHECK-NEXT: Sub-group Size: 1
; CHECK-NEXT: Min Work Width: 1
; CHECK-NEXT: Preferred Work Width: 1

; CHECK:      Kernel Name: kernel3
; CHECK-NEXT: Source Name: kernel3
; CHECK-NEXT: Local Memory: 0
; CHECK-NEXT: Sub-group Size: vscale x 1
; CHECK-NEXT: Min Work Width: 1
; CHECK-NEXT: Preferred Work Width: vscale x 1

; CHECK:      Kernel Name: kernel4
; CHECK-NEXT: Source Name: kernel4
; CHECK-NEXT: Local Memory: 0
; CHECK-NEXT: Sub-group Size: vscale x 1
; CHECK-NEXT: Min Work Width: vscale x 1
; CHECK-NEXT: Preferred Work Width: vscale x 1

; A vectorized main loop and a scalar tail
define void @kernel1() !codeplay_ca_kernel !0 !codeplay_ca_wrapper !2 {
  ret void
}

; No vectorization; no tail loop
define void @kernel2() #0 !codeplay_ca_kernel !0 {
  ret void
}

; Has a vectorized main loop and a vp-vectorized tail loop
define void @kernel3() !codeplay_ca_kernel !0 !codeplay_ca_wrapper !4 {
  ret void
}

; Has a vectorized main loop only
define void @kernel4() !codeplay_ca_kernel !0 !codeplay_ca_wrapper !6 {
  ret void
}

attributes #0 = { "mux-orig-fn"="test" }

!0 = !{i32 1}
; Fields for vectorization data are width, isScalable, SimdDimIdx, isVP
; Main vectorization of 1,S
!1 = !{i32 1, i32 1, i32 0, i32 0}
!2 = !{!1, !3}
; Tail is scalar
!3 = !{i32 1, i32 0, i32 0, i32 0}
!4 = !{!1, !5}
; Tail is 1,S, vector predicated
!5 = !{i32 1, i32 1, i32 0, i32 1}
!6 = !{!1, null}
