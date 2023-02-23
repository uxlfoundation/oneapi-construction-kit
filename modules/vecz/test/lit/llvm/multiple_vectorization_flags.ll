; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; Check some basic properties of the veczc command line interface for multiple
; vectorizations works in various configurations. The kernel outputs here are
; not interesting, only their names.
; REQUIRES: llvm-12+
; RUN: %veczc -w 8 -k foo:4,8,16.2@32s -k bar:,64s -S < %s | %filecheck %s

; CHECK-DAG: define spir_kernel void @foo
; CHECK-DAG: define spir_kernel void @bar
; CHECK-DAG: define spir_kernel void @__vecz_v4_foo
; CHECK-DAG: define spir_kernel void @__vecz_v8_foo
; CHECK-DAG: define spir_kernel void @__vecz_nxv16_foo
; CHECK-DAG: define spir_kernel void @__vecz_v8_bar
; CHECK-DAG: define spir_kernel void @__vecz_nxv64_bar

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @foo(i32 addrspace(1)* %in1, i32 addrspace(1)* %in2, i32 addrspace(1)* %out) {
entry:
  ret void
}

define spir_kernel void @bar(i32 addrspace(1)* %in1, i32 addrspace(1)* %in2, i32 addrspace(1)* %out) {
entry:
  ret void
}


