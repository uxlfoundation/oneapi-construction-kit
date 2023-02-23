; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes transfer-kernel-metadata,verify -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

; CHECK: declare spir_kernel void @foo() [[FOO_ATTRS:#[0-9]+]]
declare spir_kernel void @foo()

; CHECK: declare spir_kernel void @bar() [[BAR_ATTRS:#[0-9]+]]
declare spir_kernel void @bar()

; CHECK-DAG: attributes [[FOO_ATTRS]] = { "mux-kernel"="entry-point" "mux-orig-fn"="foo" }
; CHECK-DAG: attributes [[BAR_ATTRS]] = { "mux-kernel"="entry-point" "mux-orig-fn"="bar" }

!0 = !{i32 4, i32 5, i32 6}
