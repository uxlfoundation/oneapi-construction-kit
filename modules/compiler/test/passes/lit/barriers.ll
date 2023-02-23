; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes barriers-pass,verify -S %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"

; Check that foo has been made internal
; CHECK: define internal void @foo() #0 {
define void @foo() #0 {
  ret void
}

; Check that bar has been made internal
; CHECK: define internal void @bar() #1 {
define void @bar() #1 {
  ret void
}

; CHECK: define void @foo.mux-barrier-wrapper() [[FOO_ATTRS:#[0-9]+]] !codeplay_ca_wrapper [[WRAPPER:![0-9]+]] {
; CHECK: call void @foo

; bar's original name is baz - check we take on that name instead
; CHECK: define void @baz.mux-barrier-wrapper() [[BAZ_ATTRS:#[0-9]+]] !codeplay_ca_wrapper [[WRAPPER]] {
; CHECK: call void @bar

; CHECK-DAG: attributes [[FOO_ATTRS]] = { nounwind "mux-base-fn-name"="foo" "mux-kernel"="entry-point" }
; CHECK-DAG: attributes [[BAZ_ATTRS]] = { nounwind "mux-base-fn-name"="baz" "mux-kernel"="entry-point" }

; We have a 'main' but no 'tail' - expect null
; CHECK-DAG: [[WRAPPER]] = !{[[MAIN_INFO:![0-9]+]], null}
; CHECK-DAG: [[MAIN_INFO]] = !{i32 1, i32 0, i32 0, i32 0}

attributes #0 = { "mux-kernel"="entry-point" }
attributes #1 = { "mux-kernel"="entry-point" "mux-base-fn-name"="baz" }
