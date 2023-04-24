; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; Check that we don't crash when costing a scalable reduction
; RUN: %veczc -vecz-scalable -vecz-passes="pre-linearize" -vecz-choices=LinearizeBOSCC -S < %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @boscc_merge() {
  ret void
}
