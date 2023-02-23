; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --device "%default_device" --passes "add-fp-control<ftz>,verify" -S %s | %filecheck %s

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-unknown-elf"

; CHECK: define internal spir_kernel void @add() [[ATTRS:#[0-9]+]] {

; CHECK: define spir_kernel void @add.host-fp-control() [[WRAPPER_ATTRS:#[0-9]+]] {

define spir_kernel void @add() #0 {
  ret void
}

; check that we haven't added 'alwaysinline' to this 'noinline' kernel
; CHECK: attributes [[ATTRS]] = { noinline }
; CHECK: attributes [[WRAPPER_ATTRS]] = { noinline nounwind "mux-base-fn-name"="add" "mux-kernel"="entry-point" }

attributes #0 = { noinline "mux-kernel"="entry-point" }
