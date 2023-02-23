; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes "make-unique-func<foo>,verify" -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: define void @foo()
define void @add() #0 {
  ret void
}

attributes #0 = { optnone noinline "mux-kernel"="entry-point" }
