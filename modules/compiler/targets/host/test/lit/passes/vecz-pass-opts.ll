; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --device "%default_device" --passes "print<vecz-pass-opts>" -S %s 2>&1 | %filecheck %s
; RUN: env CODEPLAY_VECZ_CHOICES=LinearizeBOSCC,FullScalarization %muxc --device "%default_device" \
; RUN:   --passes "print<vecz-pass-opts>" -S %s 2>&1 | %filecheck %s --check-prefix CHOICES

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: Function 'foo' will be vectorized {
; CHECK:   VF = 4, (auto), vec-dim = 0, local-size = 5, choices = [
; CHECK:     DivisionExceptions
; CHECK:   ]
; CHECK: }
define spir_kernel void @foo(i32 addrspace(1)* %in) #0 !reqd_work_group_size !0 {
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  ret void
}

; CHECK: Function 'bar' will be vectorized {
; CHECK:   VF = 16, (auto), vec-dim = 0, local-size = 17, choices = [
; CHECK:     DivisionExceptions
; CHECK:   ]
; CHECK: }
define spir_kernel void @bar(i32 addrspace(1)* %in) #0 !reqd_work_group_size !1 {
  ret void
}

; CHECK: Function 'baz' will be vectorized {
; CHECK:   VF = 8, (auto), vec-dim = 0, local-size = 12, choices = [
; CHECK:     DivisionExceptions
; CHECK:   ]
; CHECK: }
define spir_kernel void @baz(i32 addrspace(1)* %in) #0 !reqd_work_group_size !2 {
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  ret void
}

; CHECK: Function 'whizz' will be vectorized {
; CHECK:   VF = 8, (auto), vec-dim = 0, local-size = 14, choices = [
; CHECK:     DivisionExceptions
; CHECK:   ]
; CHECK: }
define spir_kernel void @whizz(i32 addrspace(1)* %in) #0 !reqd_work_group_size !3 {
  ret void
}

; CHOICES: Function 'whazz' will be vectorized {
; CHOICES:   VF = 16, vec-dim = 0, local-size = 16, choices = [
; CHOICES:     LinearizeBOSCC,FullScalarization,DivisionExceptions
; CHOICES:   ]
; CHOICES: }
define spir_kernel void @whazz(i32 addrspace(1)* %in) #1 !reqd_work_group_size !4 {
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

attributes #0 = { "mux-kernel"="entry-point" "vecz-mode"="auto" }
attributes #1 = { "mux-kernel"="entry-point" "vecz-mode"="always" }

!0 = !{ i32 5, i32 1, i32 1 }
!1 = !{ i32 17, i32 1, i32 1 }
!2 = !{ i32 12, i32 1, i32 1 }
!3 = !{ i32 14, i32 1, i32 1 }
!4 = !{ i32 16, i32 1, i32 4 }
