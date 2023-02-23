; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %muxc --passes add-sched-params,verify -S %s -opaque-pointers | %filecheck %s

target triple = "spir32-unknown-unknown"
target datalayout = "e-p:32:32:32-m:e-i64:64-f80:128-n8:16:32:64-S128"

declare spir_func i64 @__mux_get_global_id(i32)

; Check that we preserve !test, but don't duplicate it

; CHECK: define spir_kernel void @foo.mux-sched-wrapper(ptr addrspace(1) %p, ptr noalias nonnull align 4 dereferenceable(24) %wi-info, ptr noalias nonnull align 4 dereferenceable(52) %wg-info)
; CHECK-SAME: [[ATTRS:#[0-9]+]] !test [[TEST_MD:\![0-9]+]] !mux_scheduled_fn [[SCHED_MD:\![0-9]+]] {
define spir_kernel void @foo(ptr addrspace(1) %p) #0 !test !0 {
  %call = tail call spir_func i64 @__mux_get_global_id(i32 0)
  ret void
}

; CHECK: attributes [[ATTRS]] = { "mux-base-fn-name"="foo" "test" "test-attr"="val" }

; CHECK: [[TEST_MD]] = !{i32 42}
; CHECK: [[SCHED_MD]] = !{i32 1, i32 2}

attributes #0 = { "test" "test-attr"="val" }

!0 = !{i32 42}
