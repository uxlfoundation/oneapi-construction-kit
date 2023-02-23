; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; REQUIRES: cl-std-3.x
; RUN: %muxc --device "%default_device" --passes "link-builtins<early>,verify" -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; Early linking builtins doesn't link in sub-group builtins
; CHECK-NOT: define{{.*}}@_Z13sub_group_alli(
declare spir_func i32 @_Z13sub_group_alli(i32)
