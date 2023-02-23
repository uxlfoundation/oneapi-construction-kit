; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --device "%default_device" --passes run-vecz -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK-LABEL: define spir_kernel void @__vecz_v4_foo(
define spir_kernel void @foo(i32 addrspace(1)* %in) #0 !reqd_work_group_size !0 {
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0)
  ret void
}

; CHECK-LABEL: define spir_kernel void @__vecz_v16_bar(
define spir_kernel void @bar(i32 addrspace(1)* %in) #0 !reqd_work_group_size !1 {
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

attributes #0 = { "mux-kernel"="entry-point" "vecz-mode"="auto" }

!0 = !{ i32 5, i32 1, i32 1 }
!1 = !{ i32 17, i32 1, i32 1 }
