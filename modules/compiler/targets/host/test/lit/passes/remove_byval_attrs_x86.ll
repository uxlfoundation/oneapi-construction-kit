; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; Run on any host device - it's the target we're compiling for that exhibits the bug
; RUN: %muxc --device "%default_device" --passes "remove-byval-attrs" -S %s -opaque-pointers | %filecheck %s

target triple = "x86_64-unknown-unknown-elf"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"

%struct.s = type { i32 }

define void @no_byval(ptr %a, ptr %b) {
  ret void
}

; CHECK: define void @byval_x(ptr %a, ptr %b, ptr %c) {
define void @byval_x(ptr %a, ptr byval(i32) %b, ptr byval(%struct.s) %c) {
  ret void
}

; CHECK: define void @byval_y(ptr %a) {
define void @byval_y(ptr byval(i32) %a) {
  ret void
}

define void @caller_byval() {
  %a = alloca i32
; CHECK: call void @byval_y(ptr %a)
  call void @byval_y(ptr byval(i32) %a)
  ret void
}
